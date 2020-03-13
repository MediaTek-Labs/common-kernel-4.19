// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/spinlock_types.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/sched/clock.h>
#include <linux/pm_runtime.h>

// power 2.0
#include "apu_log.h"
#include "apusys_power.h"
#include "apusys_power_debug.h"

// legacy power
#include "apu_dvfs.h"
#include "vpu_power_ctl.h"
#if SUPPORT_MDLA
#include "mdla_dvfs.h"
#endif
#include "spm_mtcmos_ctl.h"

int g_pwr_log_level = APUSYS_PWR_LOG_ERR;
int g_pm_procedure;
static uint64_t power_info_id;

static LIST_HEAD(power_device_list);
static LIST_HEAD(power_callback_device_list);
static struct mutex power_device_list_mtx;
static spinlock_t power_info_lock;
static uint64_t timestamp;

static struct workqueue_struct *wq;
static void d_work_power_info_func(struct work_struct *work);
static DECLARE_WORK(d_work_power_info, d_work_power_info_func);


bool apusys_power_check(void)
{
#ifdef CONFIG_MACH_MT6885
	char *pwr_ptr;
	bool pwr_status = true;

	pwr_ptr = strstr(saved_command_line,
				"apusys_status=normal");
	if (pwr_ptr == 0) {
		pwr_status = false;
		LOG_ERR("apusys power disable !!, pwr_status=%d\n",
			pwr_status);
	}
	LOG_INF("apusys power check, pwr_status=%d\n",
			pwr_status);
	return pwr_status;
#else
	return true;
#endif
}
EXPORT_SYMBOL(apusys_power_check);

struct power_device {
	enum DVFS_USER dev_usr;
	int is_power_on;
	struct platform_device *pdev;
	struct list_head list;
};

struct power_callback_device {
	enum POWER_CALLBACK_USER power_callback_usr;
	void (*power_on_callback)(void *para);
	void (*power_off_callback)(void *para);
	struct list_head list;
};

#ifdef CONFIG_PM_WAKELOCKS
struct wakeup_source *pwr_wake_lock;
#else
struct wake_lock pwr_wake_lock;
#endif

static void apu_pwr_wake_lock(void)
{
#ifdef CONFIG_PM_WAKELOCKS
	__pm_stay_awake(pwr_wake_lock);
#else
	wake_lock(&pwr_wake_lock);
#endif
}

static void apu_pwr_wake_unlock(void)
{
#ifdef CONFIG_PM_WAKELOCKS
	__pm_relax(pwr_wake_lock);
#else
	wake_unlock(&pwr_wake_lock);
#endif
}

static void apu_pwr_wake_init(void)
{
#ifdef CONFIG_PM_WAKELOCKS
	pwr_wake_lock = wakeup_source_register(NULL, "apupwr_wakelock");
#else
	wake_lock_init(&pwr_wake_lock, WAKE_LOCK_SUSPEND, "apupwr_wakelock");
#endif
}

uint64_t get_current_time_us(void)
{
	struct timeval t;

	do_gettimeofday(&t);
	return ((t.tv_sec & 0xFFF) * 1000000 + t.tv_usec);
}

uint64_t apu_get_power_info(uint8_t force)
{
	pr_info("%d %d\n", power_info_id, timestamp);
	return 0;
}
EXPORT_SYMBOL(apu_get_power_info);

static int apusys_power_off(int dev_id)
{
	if (dev_id == VPU0)
		vpu_put_power_nowq(0);
	else if (dev_id == VPU1)
		vpu_put_power_nowq(1);
#if SUPPORT_MDLA
	else if (dev_id == MDLA0)
		mdla_put_power(0);
#endif
	else
		return -1;

	return 0;
}

static int apusys_power_on(int dev_id, uint8_t opp)
{
	if (dev_id >= 0 && dev_id < APUSYS_DVFS_USER_NUM) {
		if (dev_id == VPU0) {
			// core, vvpu_index, freq_index
			vpu_opp_mapping_check(0, opp);
			// core, secure
			vpu_get_power(0, false);
		} else if (dev_id == VPU1) {
			vpu_opp_mapping_check(1, opp);
			vpu_get_power(1, false);
#if SUPPORT_MDLA
		} else if (dev_id == MDLA0) {
			mdla_opp_mapping_check(0, opp);
			mdla_get_power(0);
#endif
		} else {
			LOG_ERR("%s fail, unknown user : %d\n",
						__func__, dev_id);
		}
	}

	return 0;
}

static struct power_device *find_out_device_by_user(enum DVFS_USER user)
{
	struct power_device *pwr_dev = NULL;

	if (!list_empty(&power_device_list)) {

		list_for_each_entry(pwr_dev,
				&power_device_list, list) {

			if (pwr_dev && pwr_dev->dev_usr == user)
				return pwr_dev;
		}

	} else {
		LOG_ERR("%s empty list\n", __func__);
	}

	return NULL;
}

uint8_t apusys_boost_value_to_opp(enum DVFS_USER user, uint8_t boost_value)
{
	uint8_t opp = 0;

	if (user == VPU0 || user == VPU1) {
		opp = vpu_boost_value_to_opp(boost_value);
#if SUPPORT_MDLA
	} else if (user == MDLA0) {
		opp = mdla_boost_value_to_opp(boost_value);
#endif
	} else {
		LOG_ERR("%s, unknown user : %d\n", __func__, user);
		return -1;
	}

	LOG_INF("%s, user:%d, boost:%d -> opp:%d\n",
			__func__, user, boost_value, opp);

	return opp;

}
EXPORT_SYMBOL(apusys_boost_value_to_opp);

bool apu_get_power_on_status(enum DVFS_USER user)
{
	bool power_on_status;
	struct power_device *pwr_dev = find_out_device_by_user(user);

	if (pwr_dev == NULL)
		return false;

	power_on_status = pwr_dev->is_power_on;

	return power_on_status;
}
EXPORT_SYMBOL(apu_get_power_on_status);

static struct power_callback_device*
find_out_callback_device_by_user(enum POWER_CALLBACK_USER user)
{
	struct power_callback_device *pwr_dev = NULL;

	if (!list_empty(&power_callback_device_list)) {

		list_for_each_entry(pwr_dev,
				&power_callback_device_list, list) {

			if (pwr_dev && pwr_dev->power_callback_usr == user)
				return pwr_dev;
		}

	} else {
		LOG_ERR("%s empty list\n", __func__);
	}

	return NULL;
}

int apu_device_power_suspend(enum DVFS_USER user, int is_suspend)
{
	int ret = 0;
#if !BRING_UP
	struct power_device *pwr_dev = NULL;

	pwr_dev = find_out_device_by_user(user);
	if (pwr_dev == NULL) {
		LOG_ERR("%s fail, dev of user %d is NULL\n", __func__, user);
		return -1;
	}

	if (pwr_dev->is_power_on == 0) {
		LOG_ERR(
		"APUPWR_OFF_FAIL, not allow user:%d to pwr off twice, is_suspend:%d\n",
							user, is_suspend);
		return -ECANCELED;
	}
#endif
	LOG_DBG("%s for user:%d, is_suspend:%d\n", __func__, user, is_suspend);

	if (is_suspend)
		g_pm_procedure = 1;

	if (!g_pm_procedure)
		apu_pwr_wake_lock();

	ret = apusys_power_off(user);
#if !BRING_UP
	if (!ret)
		pwr_dev->is_power_on = 0;
#endif
	if (!g_pm_procedure)
		apu_pwr_wake_unlock();

	LOG_PM("%s for user:%d, ret:%d, is_suspend:%d, info_id: %llu\n",
					__func__, user, ret, is_suspend,
					ret ? 0 : apu_get_power_info(0));

	if (ret) {
		// apusys_reg_dump();
		apu_aee_warn("APUPWR_OFF_FAIL", "user:%d, is_suspend:%d\n",
							user, is_suspend);
		return -ENODEV;
	}

	return 0;
}
EXPORT_SYMBOL(apu_device_power_suspend);

int apu_device_power_off(enum DVFS_USER user)
{
	return apu_device_power_suspend(user, 0);
}
EXPORT_SYMBOL(apu_device_power_off);

int apu_device_power_on(enum DVFS_USER user)
{
	LOG_DBG(
	"%s bypass, real power on will be called in apu_device_set_opp\n",
								__func__);
	return 0;
}
EXPORT_SYMBOL(apu_device_power_on);

static int real_apu_device_power_on(enum DVFS_USER user, uint8_t opp)
{
	int ret = 0;
#if !BRING_UP
	struct power_device *pwr_dev = NULL;

	pwr_dev = find_out_device_by_user(user);
	if (pwr_dev == NULL) {
		LOG_ERR("%s fail, dev of user %d is NULL\n", __func__, user);
		return -1;
	}

	if (pwr_dev->is_power_on == 1) {
		LOG_ERR("APUPWR_ON_FAIL, not allow user:%d to pwr on twice\n",
									user);
		return -ECANCELED;
	}
#endif
	LOG_DBG("%s for user:%d, cnt:%d\n", __func__, user);

	g_pm_procedure = 0;

	if (!g_pm_procedure)
		apu_pwr_wake_lock();

	ret = apusys_power_on(user, opp);
#if !BRING_UP
	if (!ret)
		pwr_dev->is_power_on = 1;
#endif
	if (!g_pm_procedure)
		apu_pwr_wake_unlock();

	LOG_PM("%s for user:%d, ret:%d, info_id: %llu\n",
				__func__, user, ret,
				ret ? 0 : apu_get_power_info(0));

	if (ret) {
		// apusys_reg_dump();
		apu_aee_warn("APUPWR_ON_FAIL", "user:%d\n", user);
		return -ENODEV;
	}

	return 0;
}

int apu_power_device_register(enum DVFS_USER user, struct platform_device *pdev)
{
	struct power_device *pwr_dev = NULL;

	pwr_dev = kzalloc(sizeof(struct power_device), GFP_KERNEL);

	if (pwr_dev == NULL) {
		LOG_ERR("%s fail in dvfs user %d\n", __func__, user);
		return -1;
	}

	pwr_dev->dev_usr = user;
	pwr_dev->is_power_on = 0;
	pwr_dev->pdev = pdev;

	/* add to device link list */
	mutex_lock(&power_device_list_mtx);

	list_add_tail(&pwr_dev->list, &power_device_list);
	mutex_unlock(&power_device_list_mtx);

	LOG_INF("%s add dvfs user %d success\n", __func__, user);

	return 0;
}
EXPORT_SYMBOL(apu_power_device_register);

int apu_power_callback_device_register(enum POWER_CALLBACK_USER user,
void (*power_on_callback)(void *para), void (*power_off_callback)(void *para))
{
	struct power_callback_device *pwr_dev = NULL;

	pwr_dev = kzalloc(sizeof(struct power_callback_device), GFP_KERNEL);

	if (pwr_dev == NULL) {
		LOG_ERR("%s fail in power callback user %d\n", __func__, user);
		return -1;
	}

	pwr_dev->power_callback_usr = user;
	pwr_dev->power_on_callback = power_on_callback;
	pwr_dev->power_off_callback = power_off_callback;

	/* add to device link list */
	mutex_lock(&power_device_list_mtx);

	list_add_tail(&pwr_dev->list, &power_callback_device_list);

	mutex_unlock(&power_device_list_mtx);

	LOG_INF("%s add power callback user %d success\n", __func__, user);

	return 0;
}
EXPORT_SYMBOL(apu_power_callback_device_register);

void apu_power_device_unregister(enum DVFS_USER user)
{
	struct power_device *pwr_dev = find_out_device_by_user(user);

	mutex_lock(&power_device_list_mtx);

	/* remove from device link list */
	list_del_init(&pwr_dev->list);
	kfree(pwr_dev);

	mutex_unlock(&power_device_list_mtx);

	LOG_INF("%s remove dvfs user %d success\n", __func__, user);
}
EXPORT_SYMBOL(apu_power_device_unregister);

void apu_power_callback_device_unregister(enum POWER_CALLBACK_USER user)
{
	struct power_callback_device *pwr_dev
			= find_out_callback_device_by_user(user);

	mutex_lock(&power_device_list_mtx);

	/* remove from device link list */
	list_del_init(&pwr_dev->list);
	kfree(pwr_dev);

	mutex_unlock(&power_device_list_mtx);

	LOG_INF("%s remove power callback user %d success\n", __func__, user);
}
EXPORT_SYMBOL(apu_power_callback_device_unregister);

static void d_work_power_info_func(struct work_struct *work)
{
	spin_lock(&power_info_lock);
	LOG_WRN("%s bypass since not support DVFS\n", __func__);
	spin_unlock(&power_info_lock);
}

void apu_device_set_opp(enum DVFS_USER user, uint8_t opp)
{
	real_apu_device_power_on(user, opp);
}
EXPORT_SYMBOL(apu_device_set_opp);

static void initial_vpu_power(struct device *dev)
{
	int i;

	for (i = 0 ; i < MTK_VPU_CORE ; i++) {
		pr_info("%s for core %d\n", __func__, i);
		init_vpu_power_flag(i);
	}

	init_vpu_power_resource(dev);
}

#if SUPPORT_MDLA
static void initial_mdla_power(struct platform_device *pdev)
{
	pr_info("%s for core %d\n", __func__, 0);
	mdla_init_hw(0, pdev);
}
#endif

#if DEBUG_OPP_TEST
static void opp_test(void)
{
	vpu_opp_check(0, 0, 0); // core, vvpu_index, freq_index
	vpu_get_power(0, false); // core, secure
	vpu_opp_check(1, 0, 0);
	vpu_get_power(1, false);
	mdla_opp_check(0, 0, 0);
	mdla_get_power(0);

	vpu_put_power_nowq(0);
	vpu_put_power_nowq(1);
	mdla_put_power(0);
	pr_info("test 1 finish ##############\n");

	vpu_opp_check(0, 9, 9); // core, vvpu_index, freq_index
	vpu_get_power(0, false); // core, secure
	vpu_opp_check(1, 9, 9);
	vpu_get_power(1, false);
	mdla_opp_check(0, 9, 9);
	mdla_get_power(0);

	vpu_put_power_nowq(0);
	vpu_put_power_nowq(1);
	mdla_put_power(0);
	pr_info("test 2 finish ##############\n");

	vpu_opp_check(0, 5, 5); // core, vvpu_index, freq_index
	vpu_get_power(0, false); // core, secure
	vpu_opp_check(1, 5, 5);
	vpu_get_power(1, false);
	mdla_opp_check(0, 5, 5);
	mdla_get_power(0);

	debug_reg();

	vpu_put_power_nowq(0);
	vpu_put_power_nowq(1);
	mdla_put_power(0);
	pr_info("test 3 finish ##############\n");
}
#endif

static int apu_power_probe(struct platform_device *pdev)
{
	pr_info("%s begin\n", __func__);
	if (!apusys_power_check())
		return 0;

	udelay(1000);
	apu_dvfs_init(pdev);

	mutex_init(&power_device_list_mtx);
	spin_lock_init(&power_info_lock);
	apu_pwr_wake_init();

	wq = create_workqueue("apu_pwr_drv_wq");
	if (IS_ERR(wq)) {
		LOG_ERR("%s create power driver wq fail\n", __func__);
		goto err_exit;
	}

	queue_work(wq, &d_work_power_info);

	apusys_spm_mtcmos_init();
	pm_runtime_enable(&pdev->dev);
	initial_vpu_power(&pdev->dev);
#if SUPPORT_MDLA
	initial_mdla_power(pdev);
#endif
#if DEBUG_OPP_TEST
	opp_test();
#endif

#if PRE_POWER_ON
	vpu_opp_check(0, 0, 0); // core, vvpu_index, freq_index
	vpu_get_power(0, false); // core, secure
	vpu_opp_check(1, 0, 0);
	vpu_get_power(1, false);
#if SUPPORT_MDLA
	mdla_opp_check(0, 0, 0);
	mdla_get_power(0);
#endif
#endif
	apusys_power_debugfs_init();

	return 0;

err_exit:
	if (wq != NULL) {
		flush_workqueue(wq);
		destroy_workqueue(wq);
		kfree(wq);
		wq = NULL;
	}

	return -1;

}

static int apu_power_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int apu_power_resume(struct platform_device *pdev)
{
	return 0;
}

static int apu_power_remove(struct platform_device *pdev)
{
	apu_dvfs_remove(pdev);
	pm_runtime_disable(&pdev->dev);
	apusys_power_debugfs_exit();

	mutex_destroy(&power_device_list_mtx);

	if (wq) {
		flush_workqueue(wq);
		destroy_workqueue(wq);
		kfree(wq);
		wq = NULL;
	}

	return 0;
}


static const struct of_device_id apu_power_of_match[] = {
	{ .compatible = "mediatek,apusys_power" },
	{ /* end of list */},
};


static struct platform_driver apu_power_driver = {
	.probe	= apu_power_probe,
	.remove	= apu_power_remove,
	.suspend = apu_power_suspend,
	.resume = apu_power_resume,
	.driver = {
		.name = "apusys_power",
		.of_match_table = apu_power_of_match,
	},
};

static int __init apu_power_drv_init(void)
{
	return platform_driver_register(&apu_power_driver);
}

module_init(apu_power_drv_init)

static void __exit apu_power_drv_exit(void)
{
	platform_driver_unregister(&apu_power_driver);
}
module_exit(apu_power_drv_exit)

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("apu power driver");

