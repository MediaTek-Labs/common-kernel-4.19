// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/module.h>
#include <linux/mutex.h>

#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/dcache.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/random.h>

#include "apu_log.h"
#include "apusys_power.h"
#include "apusys_power_debug.h"
#include "apu_platform_debug.h"
#include "mdla_dvfs.h"
#include "mdla_debug.h"
#include "vpu_cmn.h"

static int g_debug_option;

int apu_power_power_stress(int type, int device, int opp)
{
	int id = 0;

	LOG_WRN("%s begin with type %d +++\n", __func__, type);

	if (type < 0 || type >= 10) {
		LOG_ERR("%s err with type = %d\n", __func__, type);
		return -1;
	}

	if (device != 9 && (device < 0 || device >= APUSYS_POWER_USER_NUM)) {
		LOG_ERR("%s err with device = %d\n", __func__, device);
		return -1;
	}

	if (type != 5 && (opp < 0 || opp >= APUSYS_MAX_NUM_OPPS)) {
		LOG_ERR("%s err with opp = %d\n", __func__, opp);
		return -1;
	}

	switch (type) {
	case 0: // config opp & run real power on flow
		if (device == 9) { // all devices
			for (id = 0 ; id < APUSYS_DVFS_USER_NUM ; id++)
				apu_device_set_opp(id, opp);
		} else {
			apu_device_set_opp(device, opp);
		}
		break;

	case 1: // config power on (bypass in legacy power project)
		if (device == 9) { // all devices
			for (id = 0 ; id < APUSYS_DVFS_USER_NUM ; id++)
				apu_device_power_on(id);
		} else {
			apu_device_power_on(device);
		}
		break;

	case 2: // config power off
		if (device == 9) { // all devices
			for (id = 0 ; id < APUSYS_DVFS_USER_NUM ; id++)
				apu_device_power_off(id);
		} else {
			apu_device_power_off(device);
		}
		break;
	default:
		LOG_WRN("%s invalid type %d !\n", __func__, type);
	}

	LOG_WRN("%s end with type %d ---\n", __func__, type);
	return 0;
}

static void change_log_level(int new_level)
{
	g_pwr_log_level = new_level;

	// FIXME: change vpu mdla power log level
	/* if (g_pwr_log_level >= APUSYS_PWR_LOG_INFO) {
	 *
	 *	mdla_klog |= MDLA_DBG_DVFS;
	 *	mdla_log_level_set(NULL, Log_STATE_MACHINE + 1);
	 *	vpu_log_level_set(NULL, Log_STATE_MACHINE + 1);
	 *	vpu_internal_log_level_set(NULL, Log_STATE_MACHINE + 1);
	 *
	 * }
	 */

	PWR_LOG_INF("%s, new log level = %d\n", __func__, g_pwr_log_level);
}

static int apusys_debug_power_show(struct seq_file *s, void *unused)
{
	switch (g_debug_option) {
	case POWER_PARAM_OPP_TABLE:
		apu_power_dump_opp_table(s);
		break;
	case POWER_PARAM_CURR_STATUS:
		apu_power_dump_curr_status(s, 0);
		break;
	case POWER_PARAM_LOG_LEVEL:
		seq_printf(s, "g_pwr_log_level = %d\n", g_pwr_log_level);
		break;
	default:
		apu_power_dump_curr_status(s, 1); // one line string
	}

	return 0;
}

static int apusys_debug_power_open(struct inode *inode, struct file *file)
{
	return single_open(file, apusys_debug_power_show, inode->i_private);
}

static int apusys_set_power_parameter(uint8_t param, int argc, int *args)
{
	int ret = 0;

	switch (param) {
	case POWER_PARAM_FIX_OPP:
		ret = (argc == 1) ? 0 : -EINVAL;
		if (ret) {
			PWR_LOG_INF(
				"invalid argument, expected:1, received:%d\n",
									argc);
			goto out;
		}

		mdla_set_power_parameter(MDLA_POWER_PARAM_FIX_OPP, argc, args);
		vpu_set_power_parameter(VPU_POWER_PARAM_FIX_OPP, argc, args);

		break;
	case POWER_PARAM_DVFS_DEBUG:
		ret = (argc == 1) ? 0 : -EINVAL;
		if (ret) {
			PWR_LOG_INF(
				"invalid argument, expected:1, received:%d\n",
									argc);
			goto out;
		}

		ret = args[0] >= APUSYS_MAX_NUM_OPPS;
		if (ret) {
			PWR_LOG_INF(
				"opp step(%d) is out-of-bound,	 max opp:%d\n",
					(int)(args[0]), APUSYS_MAX_NUM_OPPS);
			goto out;
		}

		mdla_set_power_parameter(MDLA_POWER_PARAM_DVFS_DEBUG,
								argc, args);
		vpu_set_power_parameter(VPU_POWER_PARAM_DVFS_DEBUG,
								argc, args);
		break;

	case POWER_HAL_CTL:
	{
		ret = (argc == 3) ? 0 : -EINVAL;
		if (ret) {
			PWR_LOG_INF(
			"invalid argument, expected:1, received:%d\n",
								argc);
			goto out;
		}

		if (args[0] < 0 || args[0] >= APUSYS_DVFS_USER_NUM) {
			PWR_LOG_INF("user(%d) is invalid\n",
					(int)(args[0]));
			goto out;
		}
		if (args[1] > 100 || args[1] < 0) {
			PWR_LOG_INF("min boost(%d) is out-of-bound\n",
					(int)(args[1]));
			goto out;
		}
		if (args[2] > 100 || args[2] < 0) {
			PWR_LOG_INF("max boost(%d) is out-of-bound\n",
					(int)(args[2]));
			goto out;
		}

		mdla_set_power_parameter(MDLA_POWER_HAL_CTL, argc, args);
		vpu_set_power_parameter(VPU_POWER_HAL_CTL, argc, args);

		break;
	}

	case POWER_PARAM_POWER_STRESS:
		ret = (argc == 3) ? 0 : -EINVAL;
		if (ret) {
			PWR_LOG_INF(
				"invalid argument, expected:3, received:%d\n",
				argc);
			goto out;
		}
		/*
		 * arg0 : type
		 * arg1 : device , 9 = all devices
		 * arg2 : opp
		 */
		apu_power_power_stress(args[0], args[1], args[2]);
		break;
	case POWER_PARAM_OPP_TABLE:
		ret = (argc == 1) ? 0 : -EINVAL;
		if (ret) {
			PWR_LOG_INF(
				"invalid argument, expected:1, received:%d\n",
				argc);
			goto out;
		}
		g_debug_option = POWER_PARAM_OPP_TABLE;
		break;
	case POWER_PARAM_CURR_STATUS:
		ret = (argc == 1) ? 0 : -EINVAL;
		if (ret) {
			PWR_LOG_INF(
				"invalid argument, expected:1, received:%d\n",
				argc);
			goto out;
		}
		g_debug_option = POWER_PARAM_CURR_STATUS;
		break;
	case POWER_PARAM_LOG_LEVEL:
		ret = (argc == 1) ? 0 : -EINVAL;
		if (ret) {
			PWR_LOG_INF(
				"invalid argument, expected:1, received:%d\n",
				argc);
			goto out;
		}
		if (args[0] == 9)
			g_debug_option = POWER_PARAM_LOG_LEVEL;
		else
			change_log_level(args[0]);
		break;
	default:
		PWR_LOG_INF("unsupport the power parameter:%d\n", param);
		break;
	}

out:
	return ret;
}


static ssize_t apusys_debug_power_write(struct file *flip,
		const char __user *buffer,
		size_t count, loff_t *f_pos)
{
	char *tmp, *token, *cursor;
	int ret, i, param;
	const int max_arg = 5;
	unsigned int args[max_arg];

	tmp = kzalloc(count + 1, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	ret = copy_from_user(tmp, buffer, count);
	if (ret) {
		PWR_LOG_INF("copy_from_user failed, ret=%d\n", ret);
		goto out;
	}

	tmp[count] = '\0';

	cursor = tmp;

	/* parse a command */
	token = strsep(&cursor, " ");
	if (strcmp(token, "fix_opp") == 0)
		param = POWER_PARAM_FIX_OPP;
	else if (strcmp(token, "dvfs_debug") == 0)
		param = POWER_PARAM_DVFS_DEBUG;
	else if (strcmp(token, "power_hal") == 0)
		param = POWER_HAL_CTL;
	else if (strcmp(token, "power_stress") == 0)
		param = POWER_PARAM_POWER_STRESS;
	else if (strcmp(token, "opp_table") == 0)
		param = POWER_PARAM_OPP_TABLE;
	else if (strcmp(token, "curr_status") == 0)
		param = POWER_PARAM_CURR_STATUS;
	else if (strcmp(token, "log_level") == 0)
		param = POWER_PARAM_LOG_LEVEL;
	else {
		ret = -EINVAL;
		PWR_LOG_INF("no power param[%s]!\n", token);
		goto out;
	}

	/* parse arguments */
	for (i = 0; i < max_arg && (token = strsep(&cursor, " ")); i++) {
		ret = kstrtouint(token, 10, &args[i]);
		if (ret) {
			PWR_LOG_INF("fail to parse args[%d]\n", i);
			goto out;
		}
	}

	apusys_set_power_parameter(param, i, args);

	ret = count;
out:

	kfree(tmp);
	return ret;
}

static int apusys_power_fail_open(struct inode *inode, struct file *file)
{
	return single_open(file, apusys_power_fail_show, inode->i_private);
}

static ssize_t apusys_power_fail_write(struct file *flip,
		const char __user *buffer,
		size_t count, loff_t *f_pos)
{
	return 0;
}

static const struct file_operations apusys_power_fail_fops = {
	.open = apusys_power_fail_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
	.write = apusys_power_fail_write,
};

static const struct file_operations apusys_debug_power_fops = {
	.open = apusys_debug_power_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
	.write = apusys_debug_power_write,
};

struct dentry *apusys_power_dir;

void apusys_power_debugfs_init(void)
{
	int ret;

	PWR_LOG_INF("%s\n", __func__);

	apusys_power_dir = debugfs_create_dir("apusys", NULL);

	ret = IS_ERR_OR_NULL(apusys_power_dir);
	if (ret) {
		LOG_ERR("failed to create debug dir.\n");
		return;
	}

	debugfs_create_file("power", (0644),
		apusys_power_dir, NULL, &apusys_debug_power_fops);
	debugfs_create_file("power_dump_fail_log", (0644),
		apusys_power_dir, NULL, &apusys_power_fail_fops);
}

void apusys_power_debugfs_exit(void)
{
	debugfs_remove(apusys_power_dir);
}

