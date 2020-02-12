// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/workqueue.h>
#include <linux/pm_runtime.h>

//#define ENABLE_PMQOS
#ifdef ENABLE_PMQOS
#include <linux/pm_qos.h>
#endif
#include "vpu_power_ctl.h"

#ifndef MTK_VPU_FPGA_PORTING

#if MTK_MMDVFS_ENABLE
#include <mmdvfs_mgr.h>
#endif
/*#include <mtk_pmic_info.h>*/
#endif
//#include <mtk_vcorefs_manager.h>

#include "vpu_dvfs.h"
#include "apu_dvfs.h"
#include <linux/regulator/consumer.h>

#include <linux/ktime.h>

#ifdef MTK_PERF_OBSERVER
#include <mt-plat/mtk_perfobserver.h>
#endif

#include "vpu_cmn.h"
#include "vpu_clock.h"
#include "mdla_dvfs.h"

struct vup_service_info {
	uint64_t vpu_base;
	uint64_t bin_base;
	bool is_cmd_done;
	struct mutex state_mutex;
	enum VpuCoreState state;
};

static struct vup_service_info vpu_service_cores[MTK_VPU_CORE];
struct vpu_device *vpu_device;
struct mutex power_mutex[MTK_VPU_CORE];
struct mutex power_counter_mutex[MTK_VPU_CORE];
struct mutex opp_mutex;
struct mutex power_lock_mutex;

#if ENABLE_WAKELOCK
#ifdef CONFIG_PM_WAKELOCKS
struct wakeup_source vpu_wake_lock[MTK_VPU_CORE];
#else
struct wake_lock vpu_wake_lock[MTK_VPU_CORE];
#endif
#endif
/* opp, mW */
struct VPU_OPP_INFO vpu_power_table[VPU_OPP_NUM] = {
	{VPU_OPP_0, 1165},
	{VPU_OPP_1, 1038},
	{VPU_OPP_2, 1008},
	{VPU_OPP_3, 988},
	{VPU_OPP_4, 932},
	{VPU_OPP_5, 658},
	{VPU_OPP_6, 564},
	{VPU_OPP_7, 521},
	{VPU_OPP_8, 456},
	{VPU_OPP_9, 315},
	{VPU_OPP_10, 275},
	{VPU_OPP_11, 210},
	{VPU_OPP_12, 138},


};

/* workqueue */
struct my_struct_t {
	int core;
	struct delayed_work my_work;
};

struct workqueue_struct *wq;
struct my_struct_t power_counter_work[MTK_VPU_CORE];

/* static struct workqueue_struct *opp_wq; */
DECLARE_DELAYED_WORK(opp_keep_work, vpu_opp_keep_routine);
DECLARE_DELAYED_WORK(sdsp_work, vpu_sdsp_routine);

/* power */
bool is_power_on[MTK_VPU_CORE];
bool is_power_debug_lock;
int power_counter[MTK_VPU_CORE];
bool force_change_vcore_opp[MTK_VPU_CORE];
bool force_change_vvpu_opp[MTK_VPU_CORE];
bool force_change_vmdla_opp[MTK_VPU_CORE];
bool force_change_dsp_freq[MTK_VPU_CORE];
bool change_freq_first[MTK_VPU_CORE];
bool opp_keep_flag;
bool sdsp_power_counter;
wait_queue_head_t waitq_change_vcore;
wait_queue_head_t waitq_do_core_executing;
uint8_t max_vcore_opp;
uint8_t max_vvpu_opp;
uint8_t max_dsp_freq;

/* dvfs */
struct vpu_dvfs_opps opps;
#ifdef ENABLE_PMQOS
struct pm_qos_request vpu_qos_vcore_request[MTK_VPU_CORE];
struct pm_qos_request vpu_qos_vvpu_request[MTK_VPU_CORE];
struct pm_qos_request vpu_qos_vmdla_request[MTK_VPU_CORE];
#endif

struct vpu_lock_power lock_power[VPU_OPP_PRIORIYY_NUM][MTK_VPU_CORE];
uint8_t max_opp[MTK_VPU_CORE];
uint8_t min_opp[MTK_VPU_CORE];
uint8_t max_boost[MTK_VPU_CORE];
uint8_t min_boost[MTK_VPU_CORE];
int vpu_init_done;
uint8_t segment_max_opp;
uint8_t segment_index;

/*regulator id*/
struct regulator *vvpu_reg_id; /*Check Serivce reject set init vlue*/
struct regulator *vmdla_reg_id;

inline int Map_DSP_Freq_Table(int freq_opp)
{
	int freq_value = 0;

	switch (freq_opp) {
	case 0:
	default:
		freq_value = 700;
		break;
	case 1:
		freq_value = 624;
		break;
	case 2:
		freq_value = 606;
		break;
	case 3:
		freq_value = 594;
		break;
	case 4:
		freq_value = 560;
		break;
	case 5:
		freq_value = 525;
		break;
	case 6:
		freq_value = 450;
		break;
	case 7:
		freq_value = 416;
		break;
	case 8:
		freq_value = 364;
		break;
	case 9:
		freq_value = 312;
		break;
	case 10:
		freq_value = 273;
		break;
	case 11:
		freq_value = 208;
		break;
	case 12:
		freq_value = 137;
		break;
	case 13:
		freq_value = 104;
		break;
	case 14:
		freq_value = 52;
		break;
	case 15:
		freq_value = 26;
		break;
	}

	return freq_value;
}

inline int wait_to_do_change_vcore_opp(int core)
{
	return 0;
}

bool vpu_opp_change_idle_check(int core)
{
	int i = 0;
	bool idle = true;

	for (i = 0 ; i < MTK_VPU_CORE ; i++) {
		if (i == core) {
			continue;
		} else {
			LOG_DBG("vpu test %d/%d/%d\n", core, i,
					vpu_service_cores[i].state);

			mutex_lock(&(vpu_service_cores[i].state_mutex));
			switch (vpu_service_cores[i].state) {
			case VCT_SHUTDOWN:
			case VCT_BOOTUP:
			case VCT_IDLE:
			case VCT_EXECUTING:
			case VCT_NONE:
				break;
			case VCT_VCORE_CHG:
				idle = false;
				mutex_unlock(
					&(vpu_service_cores[i].state_mutex));
				goto out;
				/*break;*/
			}
			mutex_unlock(&(vpu_service_cores[i].state_mutex));
		}
	}

out:
	return idle;
}

#ifndef MTK_VPU_FPGA_PORTING
int vpu_set_clock_source(struct clk *clk, uint8_t step)
{
	struct clk *clk_src;

	LOG_DBG("vpu scc(%d)", step);
	/* set dsp frequency - 0:700 MHz, 1:624 MHz, 2:606 MHz, 3:594 MHz*/
	/* set dsp frequency - 4:560 MHz, 5:525 MHz, 6:450 MHz, 7:416 MHz*/
	/* set dsp frequency - 8:364 MHz, 9:312 MHz, 10:273 MH, 11:208 MH*/
	/* set dsp frequency - 12:137 MHz, 13:104 MHz, 14:52 MHz, 15:26 MHz*/

	switch (step) {
	case 0:
		if (segment_index == SEGMENT_95)
			clk_src = clk_top_adsppll_d5;
		else
			clk_src = clk_top_adsppll_d4;
		break;
	case 1:
		clk_src = clk_top_univpll_d2;
		break;
	case 2:
		clk_src = clk_top_tvdpll_mainpll_d2_ck;
		break;
	case 3:
		clk_src = clk_top_tvdpll_ck;
		break;
	case 4:
		if (segment_index == SEGMENT_95)
			clk_src = clk_top_tvdpll_ck;
		else
			clk_src = clk_top_adsppll_d5;
		break;
	case 5:
		clk_src = clk_top_mmpll_d6;
		break;
	case 6:
		clk_src = clk_top_mmpll_d7;
		break;
	case 7:
		clk_src = clk_top_univpll_d3;
		break;
	case 8:
		clk_src = clk_top_mainpll_d3;
		break;
	case 9:
		clk_src = clk_top_univpll_d2_d2;
		break;
	case 10:
		clk_src = clk_top_mainpll_d2_d2;
		break;
	case 11:
		clk_src = clk_top_univpll_d3_d2;
		break;
	case 12:
		clk_src = clk_top_mainpll_d2_d4;
		break;
	case 13:
		clk_src = clk_top_univpll_d3_d4;
		break;
	case 14:
		clk_src = clk_top_univpll_d3_d8;
		break;
	case 15:
		clk_src = clk_top_clk26m;
		break;
	default:
		LOG_ERR("wrong freq step(%d)", step);
		return -EINVAL;
	}

	return clk_set_parent(clk, clk_src);
}

/*set CONN hf_fdsp_ck, VCORE hf_fipu_if_ck*/
int vpu_if_set_clock_source(struct clk *clk, uint8_t step)
{
	struct clk *clk_src;

	LOG_DBG("vpu scc(%d)", step);
	/* set dsp frequency - 0:624 MHz, 1:624 MHz, 2:606 MHz, 3:594 MHz*/
	/* set dsp frequency - 4:560 MHz, 5:525 MHz, 6:450 MHz, 7:416 MHz*/
	/* set dsp frequency - 8:364 MHz, 9:312 MHz, 10:273 MH, 11:208 MH*/
	/* set dsp frequency - 12:137 MHz, 13:104 MHz, 14:52 MHz, 15:26 MHz*/
	switch (step) {
	case 0:
		clk_src = clk_top_univpll_d2;/*624MHz*/
		break;
	case 1:
		clk_src = clk_top_univpll_d2;/*624MHz*/
		break;
	case 2:
		clk_src = clk_top_tvdpll_mainpll_d2_ck;
		break;
	case 3:
		clk_src = clk_top_tvdpll_ck;
		break;
	case 4:
		clk_src = clk_top_adsppll_d5;
		break;
	case 5:
		clk_src = clk_top_mmpll_d6;
		break;
	case 6:
		clk_src = clk_top_mmpll_d7;
		break;
	case 7:
		clk_src = clk_top_univpll_d3;
		break;
	case 8:
		clk_src = clk_top_mainpll_d3;
		break;
	case 9:
		clk_src = clk_top_univpll_d2_d2;
		break;
	case 10:
		clk_src = clk_top_mainpll_d2_d2;
		break;
	case 11:
		clk_src = clk_top_univpll_d3_d2;
		break;
	case 12:
		clk_src = clk_top_mainpll_d2_d4;
		break;
	case 13:
		clk_src = clk_top_univpll_d3_d4;
		break;
	case 14:
		clk_src = clk_top_univpll_d3_d8;
		break;
	case 15:
		clk_src = clk_top_clk26m;
		break;
	default:
		LOG_ERR("wrong freq step(%d)", step);
		return -EINVAL;
	}

	return clk_set_parent(clk, clk_src);
}
#endif


int get_vpu_opp(void)
{
	//LOG_DBG("[mdla_%d] vvpu(%d->%d)\n", core, get_vvpu_value, opp_value);
	return opps.dsp.index;
}
EXPORT_SYMBOL(get_vpu_opp);

int get_vpu_dspcore_opp(int core)
{
	LOG_DBG("[vpu_%d] get opp:%d\n", core, opps.dspcore[core].index);
	return opps.dspcore[core].index;
}
EXPORT_SYMBOL(get_vpu_dspcore_opp);

int get_vpu_platform_floor_opp(void)
{
	return (VPU_MAX_NUM_OPPS - 1);
}
EXPORT_SYMBOL(get_vpu_platform_floor_opp);

int get_vpu_ceiling_opp(int core)
{
	return max_opp[core];
}
EXPORT_SYMBOL(get_vpu_ceiling_opp);

int get_vpu_opp_to_freq(uint8_t step)
{
	int freq = 0;
	/* set dsp frequency - 0:700 MHz, 1:624 MHz, 2:606 MHz, 3:594 MHz*/
	/* set dsp frequency - 4:560 MHz, 5:525 MHz, 6:450 MHz, 7:416 MHz*/
	/* set dsp frequency - 8:364 MHz, 9:312 MHz, 10:273 MH, 11:208 MH*/
	/* set dsp frequency - 12:137 MHz, 13:104 MHz, 14:52 MHz, 15:26 MHz*/

	switch (step) {
	case 0:
		freq = 700;
		break;
	case 1:
		freq = 624;
		break;
	case 2:
		freq = 606;
		break;
	case 3:
		freq = 594;
		break;
	case 4:
		freq = 560;
		break;
	case 5:
		freq = 525;
		break;
	case 6:
		freq = 450;
		break;
	case 7:
		freq = 416;
		break;
	case 8:
		freq = 364;
		break;
	case 9:
		freq = 312;
		break;
	case 10:
		freq = 273;
		break;
	case 11:
		freq = 208;
		break;
	case 12:
		freq = 137;
		break;
	case 13:
		freq = 104;
		break;
	case 14:
		freq = 52;
		break;
	case 15:
		freq = 26;
		break;
	default:
		LOG_ERR("wrong freq step(%d)", step);
		return -EINVAL;
	}
	return freq;
}
EXPORT_SYMBOL(get_vpu_opp_to_freq);

/* expected range, vvpu_index: 0~15 */
/* expected range, freq_index: 0~15 */
//void vpu_opp_check(int core, uint8_t vcore_index, uint8_t freq_index)
void vpu_opp_check(int core, uint8_t vvpu_index, uint8_t freq_index)
{
	int i = 0;
	bool freq_check = false;
	int log_freq = 0, log_max_freq = 0;
	//int get_vcore_opp = 0;
	int get_vvpu_opp = 0;

	if (is_power_debug_lock) {
		force_change_vcore_opp[core] = false;
		force_change_vvpu_opp[core] = false;
		force_change_dsp_freq[core] = false;
		goto out;
	}

	log_freq = Map_DSP_Freq_Table(freq_index);

	LOG_DBG("opp_check + (%d/%d/%d), ori vvpu(%d)\n", core,
			vvpu_index, freq_index, opps.vvpu.index);

	mutex_lock(&opp_mutex);
	change_freq_first[core] = false;
	log_max_freq = Map_DSP_Freq_Table(max_dsp_freq);
	/*segment limitation*/
	if (vvpu_index < opps.vvpu.opp_map[segment_max_opp])
		vvpu_index = opps.vvpu.opp_map[segment_max_opp];
	if (freq_index < segment_max_opp)
		freq_index = segment_max_opp;
	/* vvpu opp*/
	get_vvpu_opp = vpu_get_hw_vvpu_opp(core);
	if (vvpu_index < opps.vvpu.opp_map[max_opp[core]])
		vvpu_index = opps.vvpu.opp_map[max_opp[core]];
	if (vvpu_index > opps.vvpu.opp_map[min_opp[core]])
		vvpu_index = opps.vvpu.opp_map[min_opp[core]];
	if (freq_index < max_opp[core])
		freq_index = max_opp[core];
	if (freq_index > min_opp[core])
		freq_index = min_opp[core];
	LOG_DVFS("opp_check + max_opp%d, min_opp%d,(%d/%d/%d), ori vvpu(%d)",
			max_opp[core], min_opp[core], core,
			vvpu_index, freq_index, opps.vvpu.index);

	//if ((vvpu_index == 0xFF) || (vvpu_index == get_vvpu_opp)) {
	if (vvpu_index == 0xFF) {
		LOG_DBG("no need, vvpu opp(%d), hw vore opp(%d)\n",
				vvpu_index, get_vvpu_opp);

		force_change_vvpu_opp[core] = false;
		opps.vvpu.index = vvpu_index;
	} else {
		/* opp down, need change freq first*/
		if (vvpu_index > get_vvpu_opp)
			change_freq_first[core] = true;

		if (vvpu_index < max_vvpu_opp) {
			LOG_INF("vpu bound vvpu opp(%d) to %d",
					vvpu_index, max_vvpu_opp);

			vvpu_index = max_vvpu_opp;
		}

		if (vvpu_index >= opps.count) {
			LOG_ERR("wrong vvpu opp(%d), max(%d)",
					vvpu_index, opps.count - 1);

		} else if ((vvpu_index < opps.vvpu.index) ||
				((vvpu_index > opps.vvpu.index) &&
				 (!opp_keep_flag)) ||
				(mdla_get_opp() < opps.dsp.index) ||
				(vvpu_index < get_vvpu_opp) ||
				((opps.dspcore[core].index < 9) &&
				 (get_vvpu_opp >= 2)) ||
				((opps.dspcore[core].index < 5) &&
				 (get_vvpu_opp >= 1))) {
			opps.vvpu.index = vvpu_index;
			force_change_vvpu_opp[core] = true;
			freq_check = true;
		}
	}

	/* dsp freq opp */
	if (freq_index == 0xFF) {
		LOG_DBG("no request, freq opp(%d)", freq_index);
		force_change_dsp_freq[core] = false;
	} else {
		if (freq_index < max_dsp_freq) {
			LOG_INF("vpu bound dsp freq(%dMHz) to %dMHz",
					log_freq, log_max_freq);
			freq_index = max_dsp_freq;
		}

		if ((opps.dspcore[core].index != freq_index) || (freq_check)) {
			/* freq_check for all vcore adjust related operation
			 * in acceptable region
			 */

			/* vcore not change and dsp change */
			//if ((force_change_vcore_opp[core] == false) &&
			if ((force_change_vvpu_opp[core] == false) &&
				(freq_index > opps.dspcore[core].index) &&
				(opp_keep_flag)) {
				if (g_vpu_log_level > Log_ALGO_OPP_INFO)
					LOG_INF("%s(%d) %s (%d/%d_%d/%d)\n",
						__func__,
						core,
						"dsp keep high",
						force_change_vvpu_opp[core],
						freq_index,
						opps.dspcore[core].index,
						opp_keep_flag);

			} else {
				opps.dspcore[core].index = freq_index;
				/*To FIX*/
				if (opps.vvpu.index == 1 &&
						opps.dspcore[core].index < 5) {
					/* adjust 0~3 to 4~7 for real table
					 * if needed
					 */
					opps.dspcore[core].index = 5;
				}
				if (opps.vvpu.index == 2 &&
						opps.dspcore[core].index < 9) {
					/* adjust 0~3 to 4~7 for real table
					 * if needed
					 */
					opps.dspcore[core].index = 9;
				}

				opps.dsp.index = 15;
				opps.ipu_if.index = 15;
				for (i = 0 ; i < MTK_VPU_CORE ; i++) {
					LOG_DBG("%s %s[%d].%s(%d->%d)\n",
						__func__,
						"opps.dspcore",
						core,
						"index",
						opps.dspcore[core].index,
						opps.dsp.index);

					/* interface should be the max freq of
					 * vpu cores
					 */
					if ((opps.dspcore[i].index <
						opps.dsp.index) &&
						(opps.dspcore[i].index >=
						 max_dsp_freq)) {

						opps.dsp.index =
							opps.dspcore[i].index;

						opps.ipu_if.index = 9;
					}
#ifdef MTK_MDLA_SUPPORT
					/*
					 * interface should be the max freq
					 * of vpu cores and mdla
					 */
					if (mdla_get_opp() < opps.dsp.index)
						LOG_DBG(
						"check mdla dsp.index\n");
						opps.dsp.index = mdla_get_opp();
						opps.ipu_if.index = 9;
#endif
					/*
					 * check opps.dsp.index and
					 * opps.vvpu.index again
					 */
					if (opps.dsp.index < 5)
						opps.vvpu.index = 0;

					if (opps.dsp.index < 9 &&
							opps.dsp.index >= 5)
						opps.vvpu.index = 1;

				}
				force_change_dsp_freq[core] = true;

				opp_keep_flag = true;
				mod_delayed_work(wq, &opp_keep_work,
					msecs_to_jiffies(OPP_KEEP_TIME_MS));
			}
		} else {
			/* vcore not change & dsp not change */
			if (g_vpu_log_level > Log_ALGO_OPP_INFO)
				LOG_INF("opp_check(%d) vcore/dsp no change\n",
						core);

			opp_keep_flag = true;

			mod_delayed_work(wq, &opp_keep_work,
					msecs_to_jiffies(OPP_KEEP_TIME_MS));
		}
	}
	mutex_unlock(&opp_mutex);
out:
	LOG_INF("%s(%d)(%d/%d_%d)(%d/%d)(%d.%d.%d.%d)(%d/%d)(%d/%d/%d/%d)%d\n",
			"opp_check",
			core,
			is_power_debug_lock,
			vvpu_index,
			freq_index,
			opps.vvpu.index,
			get_vvpu_opp,
			opps.dsp.index,
			opps.dspcore[0].index,
			opps.dspcore[1].index,
			opps.ipu_if.index,
			max_vvpu_opp,
			max_dsp_freq,
			freq_check,
			force_change_vvpu_opp[core],
			force_change_dsp_freq[core],
			change_freq_first[core],
			opp_keep_flag);
}

bool vpu_change_opp(int core, int type)
{
#ifdef MTK_VPU_FPGA_PORTING
	LOG_INF("[vpu_%d] %d Skip at FPGA", core, type);

	return true;
#else
	int ret = false;

	switch (type) {
	/* vcore opp */
	case OPPTYPE_VCORE:
		LOG_DBG("[vpu_%d] wait for changing vcore opp", core);

		ret = wait_to_do_change_vcore_opp(core);
		if (ret) {
			LOG_ERR("[vpu_%d] timeout to %s, ret=%d\n",
					core,
					"wait_to_do_change_vcore_opp",
					ret);
			goto out;
		}
		LOG_DBG("[vpu_%d] to do vcore opp change", core);
		mutex_lock(&(vpu_service_cores[core].state_mutex));
		vpu_service_cores[core].state = VCT_VCORE_CHG;
		mutex_unlock(&(vpu_service_cores[core].state_mutex));
		mutex_lock(&opp_mutex);
#ifdef ENABLE_PMQOS
		switch (opps.vcore.index) {
		case 0:
			pm_qos_update_request(&vpu_qos_vcore_request[core],
					VCORE_OPP_0);
			break;
		case 1:
			pm_qos_update_request(&vpu_qos_vcore_request[core],
					VCORE_OPP_1);
			break;
		case 2:
		default:
			pm_qos_update_request(&vpu_qos_vcore_request[core],
					VCORE_OPP_2);
			break;
		}
#else
#if MTK_MMDVFS_ENABLE
		ret = mmdvfs_set_fine_step(MMDVFS_SCEN_VPU_KERNEL,
				opps.vcore.index);
#endif
#endif
		mutex_lock(&(vpu_service_cores[core].state_mutex));
		vpu_service_cores[core].state = VCT_BOOTUP;
		mutex_unlock(&(vpu_service_cores[core].state_mutex));
		if (ret) {
			LOG_ERR("[vpu_%d]fail to request vcore, step=%d\n",
					core, opps.vcore.index);
			goto out;
		}

		LOG_DBG("[vpu_%d] cgopp vvpu=%d\n",
				core,
				regulator_get_voltage(vvpu_reg_id));

		force_change_vcore_opp[core] = false;
		mutex_unlock(&opp_mutex);
		wake_up_interruptible(&waitq_do_core_executing);
		break;
		/* dsp freq opp */
	case OPPTYPE_DSPFREQ:
		mutex_lock(&opp_mutex);
		LOG_INF("[vpu_%d] %s setclksrc(%d/%d/%d/%d)\n",
				core,
				"vpu_change_opp",
				opps.dsp.index,
				opps.dspcore[0].index,
				opps.dspcore[1].index,
				opps.ipu_if.index);

		ret = vpu_if_set_clock_source(clk_top_dsp_sel, opps.dsp.index);
		if (ret) {
			LOG_ERR("[vpu_%d]fail to set dsp freq, %s=%d, %s=%d\n",
					core,
					"step", opps.dsp.index,
					"ret", ret);
			goto out;
		}

		if (core == 0) {
			ret = vpu_set_clock_source(clk_top_dsp1_sel,
					opps.dspcore[core].index);
			if (ret) {
				LOG_ERR("[vpu_%d]%s%d freq, %s=%d, %s=%d\n",
						core, "fail to set dsp_",
						core, "step",
						opps.dspcore[core].index,
						"ret", ret);
				goto out;
			}
		} else if (core == 1) {
			ret = vpu_set_clock_source(clk_top_dsp2_sel,
					opps.dspcore[core].index);
			if (ret) {
				LOG_ERR("[vpu_%d]%s%d freq, %s=%d, %s=%d\n",
						core, "fail to set dsp_",
						core, "step",
						opps.dspcore[core].index,
						"ret", ret);
				goto out;
			}
		}

		ret = vpu_if_set_clock_source(clk_top_ipu_if_sel,
				opps.ipu_if.index);
		if (ret) {
			LOG_ERR("[vpu_%d]%s, %s=%d, %s=%d\n",
					core,
					"fail to set ipu_if freq",
					"step", opps.ipu_if.index,
					"ret", ret);
			goto out;
		}

		force_change_dsp_freq[core] = false;
		mutex_unlock(&opp_mutex);

#ifdef MTK_PERF_OBSERVER
		{
			struct pob_xpufreq_info pxi;

			pxi.id = core;
			pxi.opp = opps.dspcore[core].index;

			pob_xpufreq_update(POB_XPUFREQ_VPU, &pxi);
		}
#endif

		break;
		/* vvpu opp */
	case OPPTYPE_VVPU:
		LOG_DBG("[vpu_%d] wait for changing vvpu opp", core);
		LOG_DBG("[vpu_%d] to do vvpu opp change", core);
		mutex_lock(&(vpu_service_cores[core].state_mutex));
		vpu_service_cores[core].state = VCT_VCORE_CHG;
		mutex_unlock(&(vpu_service_cores[core].state_mutex));
		mutex_lock(&opp_mutex);
#ifdef ENABLE_PMQOS
		switch (opps.vvpu.index) {
		case 0:
			pm_qos_update_request(&vpu_qos_vcore_request[core],
					VCORE_OPP_1);
			pm_qos_update_request(&vpu_qos_vvpu_request[core],
					VVPU_OPP_0);
			pm_qos_update_request(&vpu_qos_vmdla_request[core],
					VMDLA_OPP_0);
			break;
		case 1:
			pm_qos_update_request(&vpu_qos_vvpu_request[core],
					VVPU_OPP_1);
			pm_qos_update_request(&vpu_qos_vmdla_request[core],
					VMDLA_OPP_1);
			pm_qos_update_request(&vpu_qos_vcore_request[core],
					VCORE_OPP_2);
			break;
		case 2:
		default:
			pm_qos_update_request(&vpu_qos_vmdla_request[core],
					VMDLA_OPP_2);
			pm_qos_update_request(&vpu_qos_vvpu_request[core],
					VVPU_OPP_2);
			pm_qos_update_request(&vpu_qos_vcore_request[core],
					VCORE_OPP_2);
			break;
		}
#else
#if MTK_MMDVFS_ENABLE
		ret = mmdvfs_set_fine_step(MMDVFS_SCEN_VPU_KERNEL,
				opps.vcore.index);
#endif
#endif
		mutex_lock(&(vpu_service_cores[core].state_mutex));
		vpu_service_cores[core].state = VCT_BOOTUP;
		mutex_unlock(&(vpu_service_cores[core].state_mutex));

		if (ret) {
			LOG_ERR("[vpu_%d]fail to request vcore, step=%d\n",
					core, opps.vcore.index);
			goto out;
		}

		LOG_DBG("[vpu_%d] cgopp vvpu=%d, vmdla=%d\n",
				core,
				regulator_get_voltage(vvpu_reg_id),
				regulator_get_voltage(vmdla_reg_id));

		force_change_vcore_opp[core] = false;
		force_change_vvpu_opp[core] = false;
		force_change_vmdla_opp[core] = false;
		mutex_unlock(&opp_mutex);
		wake_up_interruptible(&waitq_do_core_executing);
		break;
	default:
		LOG_DVFS("unexpected type(%d)", type);
		break;
	}

out:
	return true;
#endif
}

int32_t vpu_thermal_en_throttle_cb(uint8_t vcore_opp, uint8_t vpu_opp)
{
	int i = 0;
	int ret = 0;
	int vvpu_opp_index = 0;
	int vpu_freq_index = 0;

	if (vpu_init_done != 1)
		return ret;

	if (vpu_opp < VPU_MAX_NUM_OPPS) {
		vvpu_opp_index = opps.vvpu.opp_map[vpu_opp];
		vpu_freq_index = opps.dsp.opp_map[vpu_opp];
	} else {
		LOG_ERR("vpu_thermal_en wrong opp(%d)\n", vpu_opp);
		return -1;
	}

	LOG_INF("%s, opp(%d)->(%d/%d)\n", __func__,
			vpu_opp, vvpu_opp_index, vpu_freq_index);

	mutex_lock(&opp_mutex);
	max_dsp_freq = vpu_freq_index;
	max_vvpu_opp = vvpu_opp_index;
	mutex_unlock(&opp_mutex);
	for (i = 0 ; i < MTK_VPU_CORE ; i++) {
		mutex_lock(&opp_mutex);

		/* force change for all core under thermal request */
		opp_keep_flag = false;

		mutex_unlock(&opp_mutex);
		vpu_opp_check(i, vvpu_opp_index, vpu_freq_index);
	}

	for (i = 0 ; i < MTK_VPU_CORE ; i++) {
		if (force_change_dsp_freq[i]) {
			/* force change freq while running */
			switch (vpu_freq_index) {
			case 0:
			default:
				LOG_INF("thermal force bound freq @700MHz\n");
				break;
			case 1:
				LOG_INF("thermal force bound freq @ 624MHz\n");
				break;
			case 2:
				LOG_INF("thermal force bound freq @606MHz\n");
				break;
			case 3:
				LOG_INF("thermal force bound freq @594MHz\n");
				break;
			case 4:
				LOG_INF("thermal force bound freq @560MHz\n");
				break;
			case 5:
				LOG_INF("thermal force bound freq @525MHz\n");
				break;
			case 6:
				LOG_INF("thermal force bound freq @450MHz\n");
				break;
			case 7:
				LOG_INF("thermal force bound freq @416MHz\n");
				break;
			case 8:
				LOG_INF("thermal force bound freq @364MHz\n");
				break;
			case 9:
				LOG_INF("thermal force bound freq @312MHz\n");
				break;
			case 10:
				LOG_INF("thermal force bound freq @273MHz\n");
				break;
			case 11:
				LOG_INF("thermal force bound freq @208MHz\n");
				break;
			case 12:
				LOG_INF("thermal force bound freq @137MHz\n");
				break;
			case 13:
				LOG_INF("thermal force bound freq @104MHz\n");
				break;
			case 14:
				LOG_INF("thermal force bound freq @52MHz\n");
				break;
			case 15:
				LOG_INF("thermal force bound freq @26MHz\n");
				break;
			}
			/*vpu_change_opp(i, OPPTYPE_DSPFREQ);*/
		}

		if (force_change_vvpu_opp[i]) {
			/* vcore change should wait */
			LOG_INF("thermal force bound vcore opp to %d\n",
					vvpu_opp_index);
			/* vcore only need to change one time from
			 * thermal request
			 */
			/*if (i == 0)*/
			/*      vpu_change_opp(i, OPPTYPE_VCORE);*/
		}
	}

	return ret;
}

int32_t vpu_thermal_dis_throttle_cb(void)
{
	int ret = 0;

	if (vpu_init_done != 1)
		return ret;
	LOG_INF("%s +\n", __func__);
	mutex_lock(&opp_mutex);
	max_vcore_opp = 0;
	max_dsp_freq = 0;
	max_vvpu_opp = 0;
	mutex_unlock(&opp_mutex);
	LOG_INF("%s -\n", __func__);

	return ret;
}

int vpu_prepare_regulator_and_clock(struct device *pdev, int core)
{
	int ret = 0;
	/*enable Vvpu Vmdla*/
	/*--Get regulator handle--*/
	vvpu_reg_id = regulator_get(pdev, "vpu");
	if (!vvpu_reg_id) {
		ret = -ENOENT;
		LOG_ERR("regulator_get vvpu_reg_id failed\n");
	}
	vmdla_reg_id = regulator_get(pdev, "VMDLA");
	if (!vmdla_reg_id) {
		ret = -ENOENT;
		LOG_ERR("regulator_get vmdla_reg_id failed\n");
	}

#ifdef MTK_VPU_FPGA_PORTING
	LOG_INF("%s skip at FPGA\n", __func__);
#else

#if CCF_MTCMOS_SUPPORT
#define PREPARE_VPU_MTCMOS(clk) \
	{ \
		clk = devm_clk_get(pdev, #clk); \
		if (IS_ERR(clk)) { \
			ret = -ENOENT; \
			LOG_ERR("can not find mtcmos: %s\n", #clk); \
		} \
	}
	PREPARE_VPU_MTCMOS(mtcmos_vpu_vcore_shutdown);
	PREPARE_VPU_MTCMOS(mtcmos_vpu_conn_shutdown);
	PREPARE_VPU_MTCMOS(mtcmos_vpu_core0_shutdown);
	PREPARE_VPU_MTCMOS(mtcmos_vpu_core1_shutdown);
	//        PREPARE_VPU_MTCMOS(mtcmos_vpu_core2_shutdown);

#undef PREPARE_VPU_MTCMOS
#endif

#define PREPARE_VPU_CLK(clk) \
	{ \
		clk = devm_clk_get(pdev, #clk); \
		if (IS_ERR(clk)) { \
			ret = -ENOENT; \
			LOG_ERR("can not find clock: %s\n", #clk); \
		} else if (clk_prepare(clk)) { \
			ret = -EBADE; \
			LOG_ERR("fail to prepare clock: %s\n", #clk); \
		} \
	}

	PREPARE_VPU_CLK(clk_mmsys_gals_ipu2mm);
	PREPARE_VPU_CLK(clk_mmsys_gals_ipu12mm);
	PREPARE_VPU_CLK(clk_mmsys_gals_comm0);
	PREPARE_VPU_CLK(clk_mmsys_gals_comm1);
	PREPARE_VPU_CLK(clk_mmsys_smi_common);
	PREPARE_VPU_CLK(clk_apu_vcore_ahb_cg);
	PREPARE_VPU_CLK(clk_apu_vcore_axi_cg);
	PREPARE_VPU_CLK(clk_apu_vcore_adl_cg);
	PREPARE_VPU_CLK(clk_apu_vcore_qos_cg);
	PREPARE_VPU_CLK(clk_apu_conn_apu_cg);
	PREPARE_VPU_CLK(clk_apu_conn_ahb_cg);
	PREPARE_VPU_CLK(clk_apu_conn_axi_cg);
	PREPARE_VPU_CLK(clk_apu_conn_isp_cg);
	PREPARE_VPU_CLK(clk_apu_conn_cam_adl_cg);
	PREPARE_VPU_CLK(clk_apu_conn_img_adl_cg);
	PREPARE_VPU_CLK(clk_apu_conn_emi_26m_cg);
	PREPARE_VPU_CLK(clk_apu_conn_vpu_udi_cg);

	switch (core) {
	case 0:
	default:
		PREPARE_VPU_CLK(clk_apu_core0_jtag_cg);
		PREPARE_VPU_CLK(clk_apu_core0_axi_m_cg);
		PREPARE_VPU_CLK(clk_apu_core0_apu_cg);
		break;
	case 1:
		PREPARE_VPU_CLK(clk_apu_core1_jtag_cg);
		PREPARE_VPU_CLK(clk_apu_core1_axi_m_cg);
		PREPARE_VPU_CLK(clk_apu_core1_apu_cg);
		break;
	}

	PREPARE_VPU_CLK(clk_top_dsp_sel);
	PREPARE_VPU_CLK(clk_top_dsp1_sel);
	PREPARE_VPU_CLK(clk_top_dsp2_sel);
	PREPARE_VPU_CLK(clk_top_ipu_if_sel);
	PREPARE_VPU_CLK(clk_top_clk26m);
	PREPARE_VPU_CLK(clk_top_univpll_d3_d8);
	PREPARE_VPU_CLK(clk_top_univpll_d3_d4);
	PREPARE_VPU_CLK(clk_top_mainpll_d2_d4);
	PREPARE_VPU_CLK(clk_top_univpll_d3_d2);
	PREPARE_VPU_CLK(clk_top_mainpll_d2_d2);
	PREPARE_VPU_CLK(clk_top_univpll_d2_d2);
	PREPARE_VPU_CLK(clk_top_mainpll_d3);
	PREPARE_VPU_CLK(clk_top_univpll_d3);
	PREPARE_VPU_CLK(clk_top_mmpll_d7);
	PREPARE_VPU_CLK(clk_top_mmpll_d6);
	PREPARE_VPU_CLK(clk_top_adsppll_d5);
	PREPARE_VPU_CLK(clk_top_tvdpll_ck);
	PREPARE_VPU_CLK(clk_top_tvdpll_mainpll_d2_ck);
	PREPARE_VPU_CLK(clk_top_univpll_d2);
	PREPARE_VPU_CLK(clk_top_adsppll_d4);
	PREPARE_VPU_CLK(clk_top_mainpll_d2);
	PREPARE_VPU_CLK(clk_top_mmpll_d4);

#undef PREPARE_VPU_CLK

#endif
	return ret;
}

int vpu_enable_regulator_and_clock(int core)
{
#ifdef MTK_VPU_FPGA_PORTING
	LOG_INF("%s skip at FPGA\n", __func__);

	is_power_on[core] = true;
	force_change_vcore_opp[core] = false;
	force_change_vvpu_opp[core] = false;
	force_change_dsp_freq[core] = false;
	return 0;
#else
	int ret = 0;
	int ret1 = 0;
	int get_vcore_opp = 0;
	int get_vvpu_opp = 0;
	//bool adjust_vcore = false;
	bool adjust_vvpu = false;

	LOG_DBG("[vpu_%d] en_rc + (%d)\n", core, is_power_debug_lock);

	/*--enable regulator--*/
	ret1 = vvpu_regulator_set_mode(true);
	udelay(100);//slew rate:rising10mV/us
	if (g_vpu_log_level > Log_STATE_MACHINE)
		LOG_INF("enable vvpu ret:%d\n", ret1);
	ret1 = vmdla_regulator_set_mode(true);
	udelay(100);//slew rate:rising10mV/us
	if (g_vpu_log_level > Log_STATE_MACHINE)
		LOG_INF("enable vmdla ret:%d\n", ret1);
	vvpu_vmdla_vcore_checker();

	get_vvpu_opp = vpu_get_hw_vvpu_opp(core);
	//if (opps.vvpu.index != get_vvpu_opp)
	adjust_vvpu = true;

	if (adjust_vvpu) {
		LOG_DBG("[vpu_%d] en_rc wait for changing vcore opp", core);
		ret = wait_to_do_change_vcore_opp(core);
		if (ret) {

			/* skip change vcore in these time */
			LOG_WRN("[vpu_%d] timeout to %s(%d/%d), ret=%d\n",
					core,
					"wait_to_do_change_vcore_opp",
					opps.vcore.index,
					get_vcore_opp,
					ret);
			ret = 0;
			goto clk_on;
		}

		LOG_DBG("[vpu_%d] en_rc to do vvpu opp change", core);
#ifdef ENABLE_PMQOS
		switch (opps.vvpu.index) {
		case 0:
			pm_qos_update_request(&vpu_qos_vcore_request[core],
					VCORE_OPP_1);
			pm_qos_update_request(&vpu_qos_vvpu_request[core],
					VVPU_OPP_0);
			pm_qos_update_request(&vpu_qos_vmdla_request[core],
					VMDLA_OPP_0);
			break;
		case 1:
			pm_qos_update_request(&vpu_qos_vvpu_request[core],
					VVPU_OPP_1);
			pm_qos_update_request(&vpu_qos_vmdla_request[core],
					VMDLA_OPP_1);
			pm_qos_update_request(&vpu_qos_vcore_request[core],
					VCORE_OPP_2);
			break;
		case 2:
		default:
			pm_qos_update_request(&vpu_qos_vmdla_request[core],
					VMDLA_OPP_2);
			pm_qos_update_request(&vpu_qos_vvpu_request[core],
					VVPU_OPP_2);
			pm_qos_update_request(&vpu_qos_vcore_request[core],
					VCORE_OPP_2);

			break;
		}
#else
#if MTK_MMDVFS_ENABLE
		ret = mmdvfs_set_fine_step(MMDVFS_SCEN_VPU_KERNEL,
				opps.vcore.index);
#endif
#endif
	}
	if (ret) {
		LOG_ERR("[vpu_%d]fail to request vcore, step=%d\n",
				core, opps.vcore.index);
		goto out;
	}
	LOG_DBG("[vpu_%d] adjust(%d,%d) result vvpu=%d, vmdla=%d\n",
			core,
			adjust_vvpu,
			opps.vvpu.index,
			regulator_get_voltage(vvpu_reg_id),
			regulator_get_voltage(vmdla_reg_id));

	LOG_DBG("[vpu_%d] en_rc setmmdvfs(%d) done\n", core, opps.vcore.index);

clk_on:

#if CCF_MTCMOS_SUPPORT
#define ENABLE_VPU_MTCMOS(clk) \
	{ \
		if (clk != NULL) { \
			if (clk_prepare_enable(clk)) \
				LOG_ERR( \
				"fail to prepare&enable mtcmos:%s\n", #clk); \
		} else { \
			LOG_WRN("mtcmos not existed: %s\n", #clk); \
		} \
	}
#else
#define ENABLE_VPU_MTCMOS(func) (*(func))(STA_POWER_ON)
#endif

#define ENABLE_VPU_CLK(clk) \
	{ \
		if (clk != NULL) { \
			if (clk_enable(clk)) \
				LOG_ERR("fail to enable clock: %s\n", #clk); \
			else \
				pr_info("%s clk: %s ok\n", __func__, #clk); \
		} else { \
			LOG_WRN("clk not existed: %s\n", #clk); \
		} \
	}


	ENABLE_VPU_CLK(clk_top_dsp_sel);
	ENABLE_VPU_CLK(clk_top_ipu_if_sel);
	ENABLE_VPU_CLK(clk_top_dsp1_sel);
	ENABLE_VPU_CLK(clk_top_dsp2_sel);

	pm_runtime_get_sync(vpu_device->dev[0]);
	ENABLE_VPU_MTCMOS(mtcmos_vpu_vcore_shutdown);
	ENABLE_VPU_MTCMOS(mtcmos_vpu_conn_shutdown);
	ENABLE_VPU_MTCMOS(mtcmos_vpu_core0_shutdown);
	ENABLE_VPU_MTCMOS(mtcmos_vpu_core1_shutdown);

	udelay(500);

	ENABLE_VPU_CLK(clk_mmsys_gals_ipu2mm);
	ENABLE_VPU_CLK(clk_mmsys_gals_ipu12mm);
	ENABLE_VPU_CLK(clk_mmsys_gals_comm0);
	ENABLE_VPU_CLK(clk_mmsys_gals_comm1);
	ENABLE_VPU_CLK(clk_mmsys_smi_common);

	ENABLE_VPU_CLK(clk_apu_vcore_ahb_cg);
	ENABLE_VPU_CLK(clk_apu_vcore_axi_cg);
	ENABLE_VPU_CLK(clk_apu_vcore_adl_cg);
	ENABLE_VPU_CLK(clk_apu_vcore_qos_cg);

	/*move vcore cg ctl to atf*/
	vcore_cg_ctl(1);
	ENABLE_VPU_CLK(clk_apu_conn_apu_cg);
	ENABLE_VPU_CLK(clk_apu_conn_ahb_cg);
	ENABLE_VPU_CLK(clk_apu_conn_axi_cg);
	ENABLE_VPU_CLK(clk_apu_conn_isp_cg);
	ENABLE_VPU_CLK(clk_apu_conn_cam_adl_cg);
	ENABLE_VPU_CLK(clk_apu_conn_img_adl_cg);
	ENABLE_VPU_CLK(clk_apu_conn_emi_26m_cg);
	ENABLE_VPU_CLK(clk_apu_conn_vpu_udi_cg);
	switch (core) {
	case 0:
	default:
		ENABLE_VPU_CLK(clk_apu_core0_jtag_cg);
		ENABLE_VPU_CLK(clk_apu_core0_axi_m_cg);
		ENABLE_VPU_CLK(clk_apu_core0_apu_cg);
		break;
	case 1:
		ENABLE_VPU_CLK(clk_apu_core1_jtag_cg);
		ENABLE_VPU_CLK(clk_apu_core1_axi_m_cg);
		ENABLE_VPU_CLK(clk_apu_core1_apu_cg);
		break;
	}
#undef ENABLE_VPU_MTCMOS
#undef ENABLE_VPU_CLK

	LOG_INF("[vpu_%d] en_rc setclksrc(%d/%d/%d/%d)\n",
			core,
			opps.dsp.index,
			opps.dspcore[0].index,
			opps.dspcore[1].index,
			opps.ipu_if.index);

	ret = vpu_if_set_clock_source(clk_top_dsp_sel, opps.dsp.index);
	if (ret) {
		LOG_ERR("[vpu_%d]fail to set dsp freq, step=%d, ret=%d\n",
				core, opps.dsp.index, ret);
		goto out;
	}

	ret = vpu_set_clock_source(clk_top_dsp1_sel, opps.dspcore[0].index);
	if (ret) {
		LOG_ERR("[vpu_%d]fail to set dsp0 freq, step=%d, ret=%d\n",
				core, opps.dspcore[0].index, ret);
		goto out;
	}

	ret = vpu_set_clock_source(clk_top_dsp2_sel, opps.dspcore[1].index);
	if (ret) {
		LOG_ERR("[vpu_%d]fail to set dsp1 freq, step=%d, ret=%d\n",
				core, opps.dspcore[1].index, ret);
		goto out;
	}
	ret = vpu_if_set_clock_source(clk_top_ipu_if_sel, opps.ipu_if.index);
	if (ret) {
		LOG_ERR("[vpu_%d]fail to set ipu_if freq, step=%d, ret=%d\n",
				core, opps.ipu_if.index, ret);
		goto out;
	}

out:
	if (g_vpu_log_level > Log_STATE_MACHINE)
		apu_get_power_info_internal();
	is_power_on[core] = true;
	force_change_vcore_opp[core] = false;
	force_change_vvpu_opp[core] = false;
	force_change_dsp_freq[core] = false;
	LOG_DBG("[vpu_%d] en_rc -\n", core);

	return ret;
#endif
}

void vpu_enable_mtcmos(void)
{
#if CCF_MTCMOS_SUPPORT
#define ENABLE_VPU_MTCMOS(clk) \
	{ \
		if (clk != NULL) { \
			if (clk_prepare_enable(clk)) \
				LOG_ERR( \
				"fail to prepare&enable mtcmos:%s\n", #clk); \
		} else { \
			LOG_WRN("mtcmos not existed: %s\n", #clk); \
		} \
	}
#else
#define ENABLE_VPU_MTCMOS(func) (*(func))(STA_POWER_ON)
#endif
	pm_runtime_get_sync(vpu_device->dev[0]);
	ENABLE_VPU_MTCMOS(mtcmos_vpu_vcore_shutdown);
	ENABLE_VPU_MTCMOS(mtcmos_vpu_conn_shutdown);
	ENABLE_VPU_MTCMOS(mtcmos_vpu_core0_shutdown);
	ENABLE_VPU_MTCMOS(mtcmos_vpu_core1_shutdown);
	ENABLE_VPU_MTCMOS(mtcmos_vpu_core2_shutdown);

	udelay(500);
	/*move vcore cg ctl to atf*/
	vcore_cg_ctl(1);

#undef ENABLE_VPU_MTCMOS
}
EXPORT_SYMBOL(vpu_enable_mtcmos);

void vpu_disable_mtcmos(void)
{
#if CCF_MTCMOS_SUPPORT
#define DISABLE_VPU_MTCMOS(clk) \
	{ \
		if (clk != NULL) { \
			clk_disable_unprepare(clk); \
		} else { \
			LOG_WRN("mtcmos not existed: %s\n", #clk); \
		} \
	}
#else
#define DISABLE_VPU_MTCMOS(func) (*(func))(STA_POWER_DOWN)
#endif
	DISABLE_VPU_MTCMOS(mtcmos_vpu_core2_shutdown);
	DISABLE_VPU_MTCMOS(mtcmos_vpu_core1_shutdown);
	DISABLE_VPU_MTCMOS(mtcmos_vpu_core0_shutdown);
	DISABLE_VPU_MTCMOS(mtcmos_vpu_conn_shutdown);
	DISABLE_VPU_MTCMOS(mtcmos_vpu_vcore_shutdown);
	pm_runtime_put_sync(vpu_device->dev[0]);

#undef DISABLE_VPU_MTCMOS
}
EXPORT_SYMBOL(vpu_disable_mtcmos);

int vpu_disable_regulator_and_clock(int core)
{
	int ret = 0;
	int ret1 = 0;

#ifdef MTK_VPU_FPGA_PORTING
	LOG_INF("%s skip at FPGA\n", __func__);

	is_power_on[core] = false;
	if (!is_power_debug_lock)
		opps.dspcore[core].index = 7;

	return ret;
#else
	unsigned int smi_bus_vpu_value = 0x0;

	/* check there is un-finished transaction in bus before
	 * turning off vpu power
	 */
#ifdef MTK_VPU_SMI_DEBUG_ON
	smi_bus_vpu_value = vpu_read_smi_bus_debug(core);

	LOG_INF("[vpu_%d] dis_rc 1 (0x%x)\n", core, smi_bus_vpu_value);
	if ((int)smi_bus_vpu_value != 0) {
		mdelay(1);
		smi_bus_vpu_value = vpu_read_smi_bus_debug(core);

		LOG_INF("[vpu_%d] dis_rc again (0x%x)\n", core,
				smi_bus_vpu_value);

		if ((int)smi_bus_vpu_value != 0) {
			smi_debug_bus_hanging_detect_ext2(0x1ff, 1, 0, 1);
			vpu_aee_warn("VPU SMI CHECK",
					"core_%d fail to check smi, value=%d\n",
					core,
					smi_bus_vpu_value);
		}
	}
#else
	LOG_DVFS("[vpu_%d] dis_rc + (0x%x)\n", core, smi_bus_vpu_value);
#endif

#define DISABLE_VPU_CLK(clk) \
	{ \
		if (clk != NULL) { \
			clk_disable(clk); \
		} else { \
			LOG_WRN("clk not existed: %s\n", #clk); \
		} \
	}
	switch (core) {
	case 0:
	default:
		DISABLE_VPU_CLK(clk_apu_core0_jtag_cg);
		DISABLE_VPU_CLK(clk_apu_core0_axi_m_cg);
		DISABLE_VPU_CLK(clk_apu_core0_apu_cg);
		break;
	case 1:
		DISABLE_VPU_CLK(clk_apu_core1_jtag_cg);
		DISABLE_VPU_CLK(clk_apu_core1_axi_m_cg);
		DISABLE_VPU_CLK(clk_apu_core1_apu_cg);
		break;
	}
	DISABLE_VPU_CLK(clk_apu_conn_apu_cg);
	DISABLE_VPU_CLK(clk_apu_conn_ahb_cg);
	DISABLE_VPU_CLK(clk_apu_conn_axi_cg);
	DISABLE_VPU_CLK(clk_apu_conn_isp_cg);
	DISABLE_VPU_CLK(clk_apu_conn_cam_adl_cg);
	DISABLE_VPU_CLK(clk_apu_conn_img_adl_cg);
	DISABLE_VPU_CLK(clk_apu_conn_emi_26m_cg);
	DISABLE_VPU_CLK(clk_apu_conn_vpu_udi_cg);
	DISABLE_VPU_CLK(clk_apu_vcore_ahb_cg);
	DISABLE_VPU_CLK(clk_apu_vcore_axi_cg);
	DISABLE_VPU_CLK(clk_apu_vcore_adl_cg);
	DISABLE_VPU_CLK(clk_apu_vcore_qos_cg);
	DISABLE_VPU_CLK(clk_mmsys_gals_ipu2mm);
	DISABLE_VPU_CLK(clk_mmsys_gals_ipu12mm);
	DISABLE_VPU_CLK(clk_mmsys_gals_comm0);
	DISABLE_VPU_CLK(clk_mmsys_gals_comm1);
	DISABLE_VPU_CLK(clk_mmsys_smi_common);
	LOG_DBG("[vpu_%d] dis_rc flag4\n", core);

#if CCF_MTCMOS_SUPPORT
#define DISABLE_VPU_MTCMOS(clk) \
	{ \
		if (clk != NULL) { \
			clk_disable_unprepare(clk); \
		} else { \
			LOG_WRN("mtcmos not existed: %s\n", #clk); \
		} \
	}
#else
#define DISABLE_VPU_MTCMOS(func) (*(func))(STA_POWER_DOWN)
#endif
	DISABLE_VPU_MTCMOS(mtcmos_vpu_core1_shutdown);
	DISABLE_VPU_MTCMOS(mtcmos_vpu_core0_shutdown);
	DISABLE_VPU_MTCMOS(mtcmos_vpu_conn_shutdown);
	DISABLE_VPU_MTCMOS(mtcmos_vpu_vcore_shutdown);
	pm_runtime_put_sync(vpu_device->dev[0]);


	DISABLE_VPU_CLK(clk_top_dsp_sel);
	DISABLE_VPU_CLK(clk_top_ipu_if_sel);
	DISABLE_VPU_CLK(clk_top_dsp1_sel);
	DISABLE_VPU_CLK(clk_top_dsp2_sel);

#undef DISABLE_VPU_MTCMOS
#undef DISABLE_VPU_CLK
#ifdef ENABLE_PMQOS
	LOG_DVFS("[vpu_%d]pc0=%d, pc1=%d\n",
			core, power_counter[0], power_counter[1]);
	pm_qos_update_request(&vpu_qos_vmdla_request[core], VMDLA_OPP_2);
	pm_qos_update_request(&vpu_qos_vvpu_request[core], VVPU_OPP_2);
	pm_qos_update_request(&vpu_qos_vcore_request[core], VCORE_OPP_UNREQ);
#else
#if MTK_MMDVFS_ENABLE
	ret = mmdvfs_set_fine_step(MMDVFS_SCEN_VPU_KERNEL,
			MMDVFS_FINE_STEP_UNREQUEST);
#endif
#endif
	if (ret) {
		LOG_ERR("[vpu_%d]fail to unrequest vcore!\n", core);
		goto out;
	}
	LOG_DBG("[vpu_%d] disable result vvpu=%d\n",
			core, regulator_get_voltage(vvpu_reg_id));
out:

	/*--disable regulator--*/
	ret1 = vmdla_regulator_set_mode(false);
	udelay(100);//slew rate:rising10mV/us
	LOG_DBG("disable vmdla ret:%d\n", ret1);

	ret1 = vvpu_regulator_set_mode(false);
	udelay(100);//slew rate:rising10mV/us
	LOG_DBG("disable vvpu ret:%d\n", ret1);
	vvpu_vmdla_vcore_checker();
	is_power_on[core] = false;
	if (!is_power_debug_lock)
		opps.dspcore[core].index = 15;
	opps.dsp.index = 9;
	opps.ipu_if.index = 9;
	if (g_vpu_log_level > Log_STATE_MACHINE)
		LOG_INF("[vpu_%d] dis_rc -\n", core);
	return ret;
#endif
}

void vpu_unprepare_regulator_and_clock(void)
{

#ifdef MTK_VPU_FPGA_PORTING
	LOG_INF("%s skip at FPGA\n", __func__);
#else
#define UNPREPARE_VPU_CLK(clk) \
	{ \
		if (clk != NULL) { \
			clk_unprepare(clk); \
			clk = NULL; \
		} \
	}
	UNPREPARE_VPU_CLK(clk_apu_core0_jtag_cg);
	UNPREPARE_VPU_CLK(clk_apu_core0_axi_m_cg);
	UNPREPARE_VPU_CLK(clk_apu_core0_apu_cg);
	UNPREPARE_VPU_CLK(clk_apu_core1_jtag_cg);
	UNPREPARE_VPU_CLK(clk_apu_core1_axi_m_cg);
	UNPREPARE_VPU_CLK(clk_apu_core1_apu_cg);
	UNPREPARE_VPU_CLK(clk_apu_conn_apu_cg);
	UNPREPARE_VPU_CLK(clk_apu_conn_ahb_cg);
	UNPREPARE_VPU_CLK(clk_apu_conn_axi_cg);
	UNPREPARE_VPU_CLK(clk_apu_conn_isp_cg);
	UNPREPARE_VPU_CLK(clk_apu_conn_cam_adl_cg);
	UNPREPARE_VPU_CLK(clk_apu_conn_img_adl_cg);
	UNPREPARE_VPU_CLK(clk_apu_conn_emi_26m_cg);
	UNPREPARE_VPU_CLK(clk_apu_conn_vpu_udi_cg);
	UNPREPARE_VPU_CLK(clk_apu_vcore_ahb_cg);
	UNPREPARE_VPU_CLK(clk_apu_vcore_axi_cg);
	UNPREPARE_VPU_CLK(clk_apu_vcore_adl_cg);
	UNPREPARE_VPU_CLK(clk_apu_vcore_qos_cg);
	UNPREPARE_VPU_CLK(clk_mmsys_gals_ipu2mm);
	UNPREPARE_VPU_CLK(clk_mmsys_gals_ipu12mm);
	UNPREPARE_VPU_CLK(clk_mmsys_gals_comm0);
	UNPREPARE_VPU_CLK(clk_mmsys_gals_comm1);
	UNPREPARE_VPU_CLK(clk_mmsys_smi_common);
	UNPREPARE_VPU_CLK(clk_top_dsp_sel);
	UNPREPARE_VPU_CLK(clk_top_dsp1_sel);
	UNPREPARE_VPU_CLK(clk_top_dsp2_sel);
	UNPREPARE_VPU_CLK(clk_top_ipu_if_sel);
	UNPREPARE_VPU_CLK(clk_top_clk26m);
	UNPREPARE_VPU_CLK(clk_top_univpll_d3_d8);
	UNPREPARE_VPU_CLK(clk_top_univpll_d3_d4);
	UNPREPARE_VPU_CLK(clk_top_mainpll_d2_d4);
	UNPREPARE_VPU_CLK(clk_top_univpll_d3_d2);
	UNPREPARE_VPU_CLK(clk_top_mainpll_d2_d2);
	UNPREPARE_VPU_CLK(clk_top_univpll_d2_d2);
	UNPREPARE_VPU_CLK(clk_top_mainpll_d3);
	UNPREPARE_VPU_CLK(clk_top_univpll_d3);
	UNPREPARE_VPU_CLK(clk_top_mmpll_d7);
#undef UNPREPARE_VPU_CLK
#endif
}

int vpu_boot_up(int core, bool secure)
{
	int ret = 0;

	/*secure flag is for sdsp force shut down*/

	LOG_DBG("[vpu_%d] boot_up +\n", core);
	mutex_lock(&power_mutex[core]);
	LOG_DBG("[vpu_%d] is_power_on(%d)\n", core, is_power_on[core]);
	if (is_power_on[core]) {
		if (secure) {
			if (power_counter[core] != 1)
				LOG_ERR("vpu_%d power counter %d > 1 .\n",
						core, power_counter[core]);
			LOG_WRN("force shut down for sdsp..\n");
			mutex_unlock(&power_mutex[core]);
			vpu_shut_down(core);
			mutex_lock(&power_mutex[core]);
		} else {
			mutex_unlock(&power_mutex[core]);
			mutex_lock(&(vpu_service_cores[core].state_mutex));
			vpu_service_cores[core].state = VCT_BOOTUP;
			mutex_unlock(&(vpu_service_cores[core].state_mutex));
			wake_up_interruptible(&waitq_change_vcore);
			return POWER_ON_MAGIC;
		}
	}
	LOG_DBG("[vpu_%d] boot_up flag2\n", core);

	mutex_lock(&(vpu_service_cores[core].state_mutex));
	vpu_service_cores[core].state = VCT_BOOTUP;
	mutex_unlock(&(vpu_service_cores[core].state_mutex));
	wake_up_interruptible(&waitq_change_vcore);

	ret = vpu_enable_regulator_and_clock(core);
	if (ret) {
		LOG_ERR("[vpu_%d]fail to enable regulator or clock\n", core);
		goto out;
	}

out:

	mutex_unlock(&power_mutex[core]);

#ifdef MTK_PERF_OBSERVER
	if (!ret) {
		struct pob_xpufreq_info pxi;

		pxi.id = core;
		pxi.opp = opps.dspcore[core].index;

		pob_xpufreq_update(POB_XPUFREQ_VPU, &pxi);
	}
#endif
	return ret;
}

int vpu_get_power(int core, bool secure)
{
	int ret = 0;

	LOG_DBG("[vpu_%d/%d] gp +\n", core, power_counter[core]);
	mutex_lock(&power_counter_mutex[core]);
	power_counter[core]++;
	ret = vpu_boot_up(core, secure);
	mutex_unlock(&power_counter_mutex[core]);
	LOG_DBG("[vpu_%d/%d] gp + 2\n", core, power_counter[core]);
	if (ret == POWER_ON_MAGIC) {
		mutex_lock(&opp_mutex);
		if (change_freq_first[core]) {
			LOG_DBG("[vpu_%d] change freq first(%d)\n",
					core, change_freq_first[core]);
			/*mutex_unlock(&opp_mutex);*/
			/*mutex_lock(&opp_mutex);*/
			if (force_change_dsp_freq[core]) {
				mutex_unlock(&opp_mutex);
				/* force change freq while running */
				LOG_DBG("vpu_%d force change dsp freq", core);
				vpu_change_opp(core, OPPTYPE_DSPFREQ);
			} else {
				mutex_unlock(&opp_mutex);
			}

			mutex_lock(&opp_mutex);
			//if (force_change_vcore_opp[core]) {
			if (force_change_vvpu_opp[core]) {
				mutex_unlock(&opp_mutex);
				/* vcore change should wait */
				LOG_DBG("vpu_%d force change vvpu opp", core);
				//vpu_change_opp(core, OPPTYPE_VCORE);
				vpu_change_opp(core, OPPTYPE_VVPU);
			} else {
				mutex_unlock(&opp_mutex);
			}
		} else {
			/*mutex_unlock(&opp_mutex);*/
			/*mutex_lock(&opp_mutex);*/
			//if (force_change_vcore_opp[core]) {
			if (force_change_vvpu_opp[core]) {
				mutex_unlock(&opp_mutex);
				/* vcore change should wait */
				LOG_DBG("vpu_%d force change vcore opp", core);
				//vpu_change_opp(core, OPPTYPE_VCORE);
				vpu_change_opp(core, OPPTYPE_VVPU);
			} else {
				mutex_unlock(&opp_mutex);
			}

			mutex_lock(&opp_mutex);
			if (force_change_dsp_freq[core]) {
				mutex_unlock(&opp_mutex);
				/* force change freq while running */
				LOG_DBG("vpu_%d force change dsp freq", core);
				vpu_change_opp(core, OPPTYPE_DSPFREQ);
			} else {
				mutex_unlock(&opp_mutex);
			}
		}
	}

	if (g_vpu_log_level > Log_STATE_MACHINE)
		apu_get_power_info_internal();
	LOG_DBG("[vpu_%d/%d] gp -\n", core, power_counter[core]);

	if (ret == POWER_ON_MAGIC)
		return 0;
	else
		return ret;

}

void vpu_put_power(int core, enum VpuPowerOnType type)
{
	LOG_DBG("[vpu_%d/%d] pp +\n", core, power_counter[core]);
	mutex_lock(&power_counter_mutex[core]);
	if (--power_counter[core] == 0) {
		switch (type) {
		case VPT_PRE_ON:
			LOG_DBG("[vpu_%d] VPT_PRE_ON\n", core);
			mod_delayed_work(wq,
				&(power_counter_work[core].my_work),
				msecs_to_jiffies(10 * PWR_KEEP_TIME_MS));
			break;
		case VPT_IMT_OFF:
			LOG_INF("[vpu_%d] VPT_IMT_OFF\n", core);
			mod_delayed_work(wq,
				&(power_counter_work[core].my_work),
				msecs_to_jiffies(0));
			break;
		case VPT_ENQUE_ON:
		default:
			LOG_DBG("[vpu_%d] VPT_ENQUE_ON\n", core);
			mod_delayed_work(wq,
				&(power_counter_work[core].my_work),
				msecs_to_jiffies(PWR_KEEP_TIME_MS));
			break;
		}
	}
	mutex_unlock(&power_counter_mutex[core]);
	LOG_DBG("[vpu_%d/%d] pp -\n", core, power_counter[core]);
}

int vpu_set_power(struct vpu_user *user, struct vpu_power *power)
{
	int ret = 0;
	uint8_t vcore_opp_index = 0xFF;
	uint8_t vvpu_opp_index = 0xFF;
	uint8_t dsp_freq_index = 0xFF;
	int i = 0, core = -1;

	for (i = 0 ; i < MTK_VPU_CORE ; i++) {
		/*LOG_DBG("debug i(%d), (0x1 << i) (0x%x)", i, (0x1 << i));*/
		if (power->core == (0x1 << i)) {
			core = i;
			break;
		}
	}

	if (core >= MTK_VPU_CORE || core < 0) {
		LOG_ERR("wrong core index (0x%x/%d/%d)",
				power->core, core, MTK_VPU_CORE);
		ret = -1;
		return ret;
	}

	LOG_INF("[vpu_%d] set power opp:%d, pid=%d, tid=%d\n",
			core, power->opp_step,
			user->open_pid, user->open_tgid);

	if (power->opp_step == 0xFF) {
		vcore_opp_index = 0xFF;
		vvpu_opp_index = 0xFF;
		dsp_freq_index = 0xFF;
	} else {
		if (power->opp_step < VPU_MAX_NUM_OPPS &&
				power->opp_step >= 0) {
			vcore_opp_index = opps.vcore.opp_map[power->opp_step];
			vvpu_opp_index = opps.vvpu.opp_map[power->opp_step];
			dsp_freq_index =
				opps.dspcore[core].opp_map[power->opp_step];
		} else {
			LOG_ERR("wrong opp step (%d)", power->opp_step);
			ret = -1;
			return ret;
		}
	}
	vpu_opp_check(core, vvpu_opp_index, dsp_freq_index);
	user->power_opp = power->opp_step;

	ret = vpu_get_power(core, false);
	mutex_lock(&(vpu_service_cores[core].state_mutex));
	vpu_service_cores[core].state = VCT_IDLE;
	mutex_unlock(&(vpu_service_cores[core].state_mutex));

	/* to avoid power leakage, power on/off need be paired */
	vpu_put_power(core, VPT_PRE_ON);
	LOG_INF("[vpu_%d] %s -\n", core, __func__);
	return ret;
}

int vpu_sdsp_get_power(struct vpu_user *user)
{
	int ret = 0;
	uint8_t vcore_opp_index = 0; /*0~15, 0 is max*/
	uint8_t dsp_freq_index = 0;  /*0~15, 0 is max*/
	int core = 0;

	if (sdsp_power_counter == 0) {
		for (core = 0 ; core < MTK_VPU_CORE ; core++) {
			vpu_opp_check(core, vcore_opp_index, dsp_freq_index);

			ret = ret | vpu_get_power(core, true);
			mutex_lock(&(vpu_service_cores[core].state_mutex));
			vpu_service_cores[core].state = VCT_IDLE;
			mutex_unlock(&(vpu_service_cores[core].state_mutex));
		}
	}
	sdsp_power_counter++;
	mod_delayed_work(wq, &sdsp_work,
			msecs_to_jiffies(SDSP_KEEP_TIME_MS));

	LOG_INF("[vpu] %s -\n", __func__);
	return ret;
}

int vpu_sdsp_put_power(struct vpu_user *user)
{
	int ret = 0;
	int core = 0;

	sdsp_power_counter--;

	if (sdsp_power_counter == 0) {
		for (core = 0 ; core < MTK_VPU_CORE ; core++) {
			while (power_counter[core] != 0)
				vpu_put_power(core, VPT_IMT_OFF);

			while (is_power_on[core] == true)
				usleep_range(100, 500);

			LOG_INF("[vpu] power_counter[%d] = %d/%d -\n",
				core, power_counter[core], is_power_on[core]);

		}
	}
	mod_delayed_work(wq, &sdsp_work, msecs_to_jiffies(0));

	LOG_INF("[vpu] %s, sdsp_power_counter = %d -\n",
			__func__, sdsp_power_counter);
	return ret;
}

void vpu_power_counter_routine(struct work_struct *work)
{
	int core = 0;
	struct my_struct_t *my_work_core =
		container_of(work, struct my_struct_t, my_work.work);

	core = my_work_core->core;
	LOG_DVFS("vpu_%d counterR (%d)+\n", core, power_counter[core]);

	mutex_lock(&power_counter_mutex[core]);
	if (power_counter[core] == 0)
		vpu_shut_down(core);
	else
		LOG_DBG("vpu_%d no need this time.\n", core);
	mutex_unlock(&power_counter_mutex[core]);

	LOG_DVFS("vpu_%d counterR -", core);
}

int vpu_quick_suspend(int core)
{
	LOG_DBG("[vpu_%d] q_suspend +\n", core);
	mutex_lock(&power_counter_mutex[core]);
	LOG_INF("[vpu_%d] q_suspend (%d/%d)\n", core,
			power_counter[core], vpu_service_cores[core].state);

	if (power_counter[core] == 0) {
		mutex_unlock(&power_counter_mutex[core]);

		mutex_lock(&(vpu_service_cores[core].state_mutex));

		switch (vpu_service_cores[core].state) {
		case VCT_SHUTDOWN:
		case VCT_NONE:
			/* vpu has already been shut down, do nothing*/
			mutex_unlock(&(vpu_service_cores[core].state_mutex));
			break;
		case VCT_IDLE:
		case VCT_BOOTUP:
		case VCT_EXECUTING:
		case VCT_VCORE_CHG:
		default:
			mutex_unlock(&(vpu_service_cores[core].state_mutex));
			mod_delayed_work(wq,
					&(power_counter_work[core].my_work),
					msecs_to_jiffies(0));
			break;
		}
	} else {
		mutex_unlock(&power_counter_mutex[core]);
	}
	return 0;
}

void vpu_opp_keep_routine(struct work_struct *work)
{
	LOG_DVFS("%s flag (%d) +\n", __func__, opp_keep_flag);
	mutex_lock(&opp_mutex);
	opp_keep_flag = false;
	mutex_unlock(&opp_mutex);
	LOG_DVFS("%s flag (%d) -\n", __func__, opp_keep_flag);
}

void vpu_sdsp_routine(struct work_struct *work)
{

	if (sdsp_power_counter != 0) {
		LOG_INF("%s long time not unlock!!! -\n", __func__);
		LOG_ERR("%s long time not unlock error!!! -\n", __func__);
	} else {
		LOG_INF("%s sdsp_power_counter is correct!!! -\n", __func__);
	}
}

int vpu_shut_down(int core)
{
	int ret = 0;

	LOG_DBG("[vpu_%d] shutdown +\n", core);
	mutex_lock(&power_mutex[core]);
	if (!is_power_on[core]) {
		mutex_unlock(&power_mutex[core]);
		return 0;
	}

	mutex_lock(&(vpu_service_cores[core].state_mutex));
	switch (vpu_service_cores[core].state) {
	case VCT_SHUTDOWN:
	case VCT_IDLE:
	case VCT_NONE:
#ifdef MTK_VPU_FPGA_PORTING
	case VCT_BOOTUP:
#endif
		vpu_service_cores[core].state = VCT_SHUTDOWN;
		mutex_unlock(&(vpu_service_cores[core].state_mutex));
		break;
#ifndef MTK_VPU_FPGA_PORTING
	case VCT_BOOTUP:
#endif
	case VCT_EXECUTING:
	case VCT_VCORE_CHG:
		mutex_unlock(&(vpu_service_cores[core].state_mutex));
		goto out;
		/*break;*/
	}

	ret = vpu_disable_regulator_and_clock(core);
	if (ret) {
		LOG_ERR("[vpu_%d]fail to disable regulator and clock\n", core);
		goto out;
	}

	wake_up_interruptible(&waitq_change_vcore);
out:
	mutex_unlock(&power_mutex[core]);
	LOG_DBG("[vpu_%d] shutdown -\n", core);

#ifdef MTK_PERF_OBSERVER
	if (!ret) {
		struct pob_xpufreq_info pxi;

		pxi.id = core;
		pxi.opp = -1;

		pob_xpufreq_update(POB_XPUFREQ_VPU, &pxi);
	}
#endif
	return ret;
}

int vpu_dump_opp_table(struct seq_file *s)
{
	int i;

#define LINE_BAR \
	"  +-----+----------+----------+------------+-----------+-----------+\n"
	vpu_print_seq(s, LINE_BAR);
	vpu_print_seq(s, "  |%-5s|%-10s|%-10s|%-10s|%-10s|%-12s|%-11s|%-11s|\n",
			"OPP", "VCORE(uV)", "VVPU(uV)",
			"VMDLA(uV)", "DSP(KHz)",
			"IPU_IF(KHz)", "DSP1(KHz)", "DSP2(KHz)");
	vpu_print_seq(s, LINE_BAR);

	for (i = 0; i < opps.count; i++) {
		vpu_print_seq(s,
			"  |%-5d|[%d]%-7d|[%d]%-7d|[%d]%-7d|[%d]%-7d|[%d]%-9d|[%d]%-8d|[%d]%-8d|\n",
			i,
			opps.vcore.opp_map[i],
			opps.vcore.values[opps.vcore.opp_map[i]],
			opps.vvpu.opp_map[i],
			opps.vvpu.values[opps.vvpu.opp_map[i]],
			opps.vmdla.opp_map[i],
			opps.vmdla.values[opps.vmdla.opp_map[i]],
			opps.dsp.opp_map[i],
			opps.dsp.values[opps.dsp.opp_map[i]],
			opps.ipu_if.opp_map[i],
			opps.ipu_if.values[opps.ipu_if.opp_map[i]],
			opps.dspcore[0].opp_map[i],
			opps.dspcore[0].values[opps.dspcore[0].opp_map[i]],
			opps.dspcore[1].opp_map[i],
			opps.dspcore[1].values[opps.dspcore[1].opp_map[i]]);
	}

	vpu_print_seq(s, LINE_BAR);
#undef LINE_BAR

	return 0;
}

int vpu_dump_power(struct seq_file *s)
{
	int vvpu_opp = 0;


	vvpu_opp = vpu_get_hw_vvpu_opp(0);


	vpu_print_seq(s, "%s(rw): %s[%d/%d]\n",
			"dvfs_debug",
			"vvpu", opps.vvpu.index, vvpu_opp);
	vpu_print_seq(s, "%s[%d], %s[%d], %s[%d], %s[%d]\n",
			"dsp", opps.dsp.index,
			"ipu_if", opps.ipu_if.index,
			"dsp1", opps.dspcore[0].index,
			"dsp2", opps.dspcore[1].index);
	vpu_print_seq(s, "min/max boost[0][%d/%d], min/max opp[1][%d/%d]\n",
			min_boost[0], max_boost[0], min_boost[1], max_boost[1]);

	vpu_print_seq(s, "min/max opp[0][%d/%d], min/max opp[1][%d/%d]\n",
			min_opp[0], max_opp[0], min_opp[1], max_opp[1]);

	vpu_print_seq(s, "is_power_debug_lock(rw): %d\n", is_power_debug_lock);

	return 0;
}

int vpu_set_power_parameter(uint8_t param, int argc, int *args)
{
	int ret = 0;

	switch (param) {
	case VPU_POWER_PARAM_FIX_OPP:
		{
			LOG_INF("VPUDBG VPU_POWER_PARAM_FIX_OPP\n");
			break;
		}
	case VPU_POWER_PARAM_DVFS_DEBUG:
		{
			LOG_INF("VPUDBG VPU_POWER_PARAM_DVFS_DEBUG\n");
			break;
		}
	case VPU_POWER_PARAM_LOCK:
		{
			LOG_INF("VPUDBG VPU_POWER_PARAM_LOCK\n");
			break;
		}
	case VPU_POWER_HAL_CTL:
		{
			LOG_INF("VPUDBG VPU_POWER_HAL_CTL\n");
			break;
		}
	case VPU_EARA_CTL:
		{
			LOG_INF("VPUDBG VPU_EARA_CTL\n");
			break;
		}
	case VPU_CT_INFO:
		{
			LOG_INF("VPUDBG VPU_CT_INFO\n");
			break;
		}
	default:
		LOG_INF("VPUDBG VPU_ERROR_PARAMETER\n");
		break;
	}

	switch (param) {
	case VPU_POWER_PARAM_FIX_OPP:
		ret = (argc == 1) ? 0 : -EINVAL;
		if (ret) {
			LOG_ERR("invalid argument, expected:1, received:%d\n",
					argc);
			goto out;
		}

		switch (args[0]) {
		case 0:
			is_power_debug_lock = false;
			break;
		case 1:
			is_power_debug_lock = true;
			break;
		default:
			if (ret) {
				LOG_ERR("invalid argument, received:%d\n",
						(int)(args[0]));
				goto out;
			}
			ret = -EINVAL;
			goto out;
		}
		break;
	case VPU_POWER_PARAM_DVFS_DEBUG:
		ret = (argc == 1) ? 0 : -EINVAL;
		if (ret) {
			LOG_ERR("invalid argument, expected:1, received:%d\n",
					argc);
			goto out;
		}

		ret = args[0] >= opps.count;
		if (ret) {
			LOG_ERR("opp step(%d) is out-of-bound, count:%d\n",
					(int)(args[0]), opps.count);
			goto out;
		}

		opps.vcore.index = opps.vcore.opp_map[args[0]];
		opps.vvpu.index = opps.vvpu.opp_map[args[0]];
		opps.vmdla.index = opps.vmdla.opp_map[args[0]];
		opps.dsp.index = opps.dsp.opp_map[args[0]];
		opps.ipu_if.index = opps.ipu_if.opp_map[args[0]];
		opps.dspcore[0].index = opps.dspcore[0].opp_map[args[0]];
		opps.dspcore[1].index = opps.dspcore[1].opp_map[args[0]];

		is_power_debug_lock = true;

		break;
	case VPU_POWER_PARAM_LOCK:
		ret = (argc == 1) ? 0 : -EINVAL;
		if (ret) {
			LOG_ERR("invalid argument, expected:1, received:%d\n",
					argc);
			goto out;
		}

		is_power_debug_lock = args[0];

		break;
	case VPU_POWER_HAL_CTL:
	{
		struct vpu_lock_power vpu_lock_power;

		ret = (argc == 3) ? 0 : -EINVAL;
		if (ret) {
			LOG_ERR("invalid argument, expected:3, received:%d\n",
					argc);
			goto out;
		}

		if (args[0] > MTK_VPU_CORE || args[0] < 1) {
			LOG_ERR("core(%d) is out-of-bound\n",
					(int)(args[0]));
			goto out;
		}

		if (args[1] > 100 || args[1] < 0) {
			LOG_ERR("min boost(%d) is out-of-bound\n",
					(int)(args[1]));
			goto out;
		}
		if (args[2] > 100 || args[2] < 0) {
			LOG_ERR("max boost(%d) is out-of-bound\n",
					(int)(args[2]));
			goto out;
		}
		vpu_lock_power.core = args[0];
		vpu_lock_power.lock = true;
		vpu_lock_power.priority = POWER_HAL;
		vpu_lock_power.max_boost_value = args[2];
		vpu_lock_power.min_boost_value = args[1];
		LOG_INF("[vpu]POWER_HAL_LOCK+core:%d, maxb:%d, minb:%d\n",
			vpu_lock_power.core, vpu_lock_power.max_boost_value,
			vpu_lock_power.min_boost_value);
		ret = vpu_lock_set_power(&vpu_lock_power);
		if (ret) {
			LOG_ERR("[POWER_HAL_LOCK]failed, ret=%d\n", ret);
			goto out;
		}
		break;
	}
	case VPU_EARA_CTL:
	{
		struct vpu_lock_power vpu_lock_power;

		ret = (argc == 3) ? 0 : -EINVAL;
		if (ret) {
			LOG_ERR("invalid argument, expected:3, received:%d\n",
					argc);
			goto out;
		}
		if (args[0] > MTK_VPU_CORE || args[0] < 1) {
			LOG_ERR("core(%d) is out-of-bound\n",
					(int)(args[0]));
			goto out;
		}

		if (args[1] > 100 || args[1] < 0) {
			LOG_ERR("min boost(%d) is out-of-bound\n",
					(int)(args[1]));
			goto out;
		}
		if (args[2] > 100 || args[2] < 0) {
			LOG_ERR("max boost(%d) is out-of-bound\n",
					(int)(args[2]));
			goto out;
		}
		vpu_lock_power.core = args[0];
		vpu_lock_power.lock = true;
		vpu_lock_power.priority = EARA_QOS;
		vpu_lock_power.max_boost_value = args[2];
		vpu_lock_power.min_boost_value = args[1];
		LOG_INF("[vpu]EARA_LOCK+core:%d, maxb:%d, minb:%d\n",
			vpu_lock_power.core, vpu_lock_power.max_boost_value,
			vpu_lock_power.min_boost_value);
		ret = vpu_lock_set_power(&vpu_lock_power);
		if (ret) {
			LOG_ERR("[POWER_HAL_LOCK]failed, ret=%d\n", ret);
			goto out;
		}
		break;
	}
	case VPU_CT_INFO:
		ret = (argc == 1) ? 0 : -EINVAL;
		if (ret) {
			LOG_ERR("invalid argument, expected:1, received:%d\n",
					argc);
			goto out;
		}
		apu_dvfs_dump_info();
		break;
	default:
		LOG_ERR("unsupport the power parameter:%d\n", param);
		break;
	}

out:
	return ret;
}

uint8_t vpu_boost_value_to_opp(uint8_t boost_value)
{
	int ret = 0;
	/* set dsp frequency - 0:700 MHz, 1:624 MHz, 2:606 MHz, 3:594 MHz*/
	/* set dsp frequency - 4:560 MHz, 5:525 MHz, 6:450 MHz, 7:416 MHz*/
	/* set dsp frequency - 8:364 MHz, 9:312 MHz, 10:273 MH, 11:208 MH*/
	/* set dsp frequency - 12:137 MHz, 13:104 MHz, 14:52 MHz, 15:26 MHz*/

	uint32_t freq = 0;
	uint32_t freq0 = opps.dspcore[0].values[0];
	uint32_t freq1 = opps.dspcore[0].values[1];
	uint32_t freq2 = opps.dspcore[0].values[2];
	uint32_t freq3 = opps.dspcore[0].values[3];
	uint32_t freq4 = opps.dspcore[0].values[4];
	uint32_t freq5 = opps.dspcore[0].values[5];
	uint32_t freq6 = opps.dspcore[0].values[6];
	uint32_t freq7 = opps.dspcore[0].values[7];
	uint32_t freq8 = opps.dspcore[0].values[8];
	uint32_t freq9 = opps.dspcore[0].values[9];
	uint32_t freq10 = opps.dspcore[0].values[10];
	uint32_t freq11 = opps.dspcore[0].values[11];
	uint32_t freq12 = opps.dspcore[0].values[12];
	uint32_t freq13 = opps.dspcore[0].values[13];
	uint32_t freq14 = opps.dspcore[0].values[14];
	uint32_t freq15 = opps.dspcore[0].values[15];

	if ((boost_value <= 100) && (boost_value >= 0))
		freq = boost_value * freq0 / 100;
	else
		freq = freq0;

	if (freq <= freq0 && freq > freq1)
		ret = 0;
	else if (freq <= freq1 && freq > freq2)
		ret = 1;
	else if (freq <= freq2 && freq > freq3)
		ret = 2;
	else if (freq <= freq3 && freq > freq4)
		ret = 3;
	else if (freq <= freq4 && freq > freq5)
		ret = 4;
	else if (freq <= freq5 && freq > freq6)
		ret = 5;
	else if (freq <= freq6 && freq > freq7)
		ret = 6;
	else if (freq <= freq7 && freq > freq8)
		ret = 7;
	else if (freq <= freq8 && freq > freq9)
		ret = 8;
	else if (freq <= freq9 && freq > freq10)
		ret = 9;
	else if (freq <= freq10 && freq > freq11)
		ret = 10;
	else if (freq <= freq11 && freq > freq12)
		ret = 11;
	else if (freq <= freq12 && freq > freq13)
		ret = 12;
	else if (freq <= freq13 && freq > freq14)
		ret = 13;
	else if (freq <= freq14 && freq > freq15)
		ret = 14;
	else
		ret = 15;

	LOG_DVFS("%s opp %d\n", __func__, ret);
	return ret;
}

bool vpu_update_lock_power_parameter(struct vpu_lock_power *vpu_lock_power)
{
	bool ret = true;
	int i, core = -1;
	unsigned int priority = vpu_lock_power->priority;

	for (i = 0 ; i < MTK_VPU_CORE ; i++) {
		if (vpu_lock_power->core == (0x1 << i)) {
			core = i;
			break;
		}
	}

	if (core >= MTK_VPU_CORE || core < 0) {
		LOG_ERR("wrong core index (0x%x/%d/%d)",
				vpu_lock_power->core, core, MTK_VPU_CORE);
		ret = false;
		return ret;
	}
	lock_power[priority][core].core = core;
	lock_power[priority][core].max_boost_value =
		vpu_lock_power->max_boost_value;
	lock_power[priority][core].min_boost_value =
		vpu_lock_power->min_boost_value;
	lock_power[priority][core].lock = true;
	lock_power[priority][core].priority =
		vpu_lock_power->priority;
	LOG_INF("power_parameter core %d, maxb:%d, minb:%d priority %d\n",
			lock_power[priority][core].core,
			lock_power[priority][core].max_boost_value,
			lock_power[priority][core].min_boost_value,
			lock_power[priority][core].priority);
	return ret;
}

bool vpu_update_unlock_power_parameter(struct vpu_lock_power *vpu_lock_power)
{
	bool ret = true;
	int i, core = -1;
	unsigned int priority = vpu_lock_power->priority;

	for (i = 0 ; i < MTK_VPU_CORE ; i++) {
		if (vpu_lock_power->core == (0x1 << i)) {
			core = i;
			break;
		}
	}

	if (core >= MTK_VPU_CORE || core < 0) {
		LOG_ERR("wrong core index (0x%x/%d/%d)",
				vpu_lock_power->core, core, MTK_VPU_CORE);
		ret = false;
		return ret;
	}
	lock_power[priority][core].core =
		vpu_lock_power->core;
	lock_power[priority][core].max_boost_value =
		vpu_lock_power->max_boost_value;
	lock_power[priority][core].min_boost_value =
		vpu_lock_power->min_boost_value;
	lock_power[priority][core].lock = false;
	lock_power[priority][core].priority =
		vpu_lock_power->priority;
	LOG_INF("%s\n", __func__);
	return ret;
}

static int8_t min_of(uint8_t value1, uint8_t value2)
{
	if (value1 <= value2)
		return value1;
	else
		return value2;
}
static uint8_t max_of(uint8_t value1, uint8_t value2)
{
	if (value1 <= value2)
		return value2;
	else
		return value1;
}

bool vpu_update_max_opp(struct vpu_lock_power *vpu_lock_power)
{
	bool ret = true;
	int i, core = -1;
	uint8_t first_priority = NORMAL;
	uint8_t first_priority_max_boost_value = 100;
	uint8_t first_priority_min_boost_value = 0;
	uint8_t temp_max_boost_value = 100;
	uint8_t temp_min_boost_value = 0;
	bool lock = false;
	uint8_t priority = NORMAL;
	uint8_t maxboost = 100;
	uint8_t minboost = 0;

	for (i = 0 ; i < MTK_VPU_CORE ; i++) {
		if (vpu_lock_power->core == (0x1 << i)) {
			core = i;
			break;
		}
	}

	if (core >= MTK_VPU_CORE || core < 0) {
		LOG_ERR("wrong core index (0x%x/%d/%d)",
				vpu_lock_power->core, core, MTK_VPU_CORE);
		ret = false;
		return ret;
	}

	for (i = 0 ; i < VPU_OPP_PRIORIYY_NUM ; i++) {
		if (lock_power[i][core].lock == true
				&& lock_power[i][core].priority < NORMAL) {
			first_priority = i;
			first_priority_max_boost_value =
				lock_power[i][core].max_boost_value;
			first_priority_min_boost_value =
				lock_power[i][core].min_boost_value;
			break;
		}
	}
	temp_max_boost_value = first_priority_max_boost_value;
	temp_min_boost_value = first_priority_min_boost_value;
	/*find final_max_boost_value*/
	for (i = first_priority ; i < VPU_OPP_PRIORIYY_NUM ; i++) {
		lock = lock_power[i][core].lock;
		priority = lock_power[i][core].priority;
		maxboost = lock_power[i][core].max_boost_value;
		minboost = lock_power[i][core].min_boost_value;
		if (lock == true
			&& priority < NORMAL &&
			(((maxboost <= temp_max_boost_value)
			&& (maxboost >= temp_min_boost_value))
			|| ((minboost <= temp_max_boost_value)
			&& (minboost >= temp_min_boost_value))
			|| ((maxboost >= temp_max_boost_value)
			&& (minboost <= temp_min_boost_value)))) {

			temp_max_boost_value =
				min_of(temp_max_boost_value,
					lock_power[i][core].max_boost_value);
			temp_min_boost_value =
				max_of(temp_min_boost_value,
					lock_power[i][core].min_boost_value);

		}
	}
	max_boost[core] = temp_max_boost_value;
	min_boost[core] = temp_min_boost_value;
	max_opp[core] =
		vpu_boost_value_to_opp(temp_max_boost_value);
	min_opp[core] =
		vpu_boost_value_to_opp(temp_min_boost_value);
	LOG_DVFS("final_min_boost_value:%d final_max_boost_value:%d\n",
			temp_min_boost_value, temp_max_boost_value);
	return ret;
}

int vpu_lock_set_power(struct vpu_lock_power *vpu_lock_power)
{
	int ret = -1;
	int i, core = -1;

	mutex_lock(&power_lock_mutex);
	for (i = 0 ; i < MTK_VPU_CORE ; i++) {
		if (vpu_lock_power->core == (0x1 << i)) {
			core = i;
			break;
		}
	}

	if (core >= MTK_VPU_CORE || core < 0) {
		LOG_ERR("wrong core index (0x%x/%d/%d)",
				vpu_lock_power->core, core, MTK_VPU_CORE);
		ret = -1;
		mutex_unlock(&power_lock_mutex);
		return ret;
	}


	if (!vpu_update_lock_power_parameter(vpu_lock_power)) {
		mutex_unlock(&power_lock_mutex);
		return -1;
	}
	if (!vpu_update_max_opp(vpu_lock_power)) {
		mutex_unlock(&power_lock_mutex);
		return -1;
	}

	mutex_unlock(&power_lock_mutex);
	return 0;
}

int vpu_unlock_set_power(struct vpu_lock_power *vpu_lock_power)
{
	int ret = -1;
	int i, core = -1;

	mutex_lock(&power_lock_mutex);
	for (i = 0 ; i < MTK_VPU_CORE ; i++) {
		if (vpu_lock_power->core == (0x1 << i)) {
			core = i;
			break;
		}
	}

	if (core >= MTK_VPU_CORE || core < 0) {
		LOG_ERR("wrong core index (0x%x/%d/%d)",
				vpu_lock_power->core, core, MTK_VPU_CORE);
		ret = false;
		return ret;
	}
	if (!vpu_update_unlock_power_parameter(vpu_lock_power)) {
		mutex_unlock(&power_lock_mutex);
		return -1;
	}
	if (!vpu_update_max_opp(vpu_lock_power)) {
		mutex_unlock(&power_lock_mutex);
		return -1;
	}
	mutex_unlock(&power_lock_mutex);
	return ret;

}

void init_vpu_power_flag(int core)
{
	sdsp_power_counter = 0;

	if (core == 0) {
		vpu_device = kzalloc(sizeof(struct vpu_device), GFP_KERNEL);
		if (!vpu_device) {
			LOG_ERR("%s, kzalloc vpu_device fail\n", __func__);
			goto err;
		}

		mutex_init(&opp_mutex);
		mutex_init(&power_lock_mutex);
		init_waitqueue_head(&waitq_change_vcore);
		init_waitqueue_head(&waitq_do_core_executing);
		max_vcore_opp = 0;
		max_vvpu_opp = 0;
		max_dsp_freq = 0;
		opp_keep_flag = false;
		is_power_debug_lock = false;
	}

	mutex_init(&(power_mutex[core]));
	mutex_init(&(power_counter_mutex[core]));
	power_counter[core] = 0;
	power_counter_work[core].core = core;
	is_power_on[core] = false;
	force_change_vcore_opp[core] = false;
	force_change_vvpu_opp[core] = false;
	force_change_dsp_freq[core] = false;
	change_freq_first[core] = false;

	INIT_DELAYED_WORK(&(power_counter_work[core].my_work),
			vpu_power_counter_routine);

#if ENABLE_WAKELOCK
#ifdef CONFIG_PM_WAKELOCKS
	if (core == 0) {
		wakeup_source_init(&(vpu_wake_lock[core]),
				"vpu_wakelock_0");
	} else {
		wakeup_source_init(&(vpu_wake_lock[core]),
				"vpu_wakelock_1");
	}
#else
	if (core == 0) {
		wake_lock_init(&(vpu_wake_lock[core]),
				WAKE_LOCK_SUSPEND,
				"vpu_wakelock_0");
	} else {
		wake_lock_init(&(vpu_wake_lock[core]),
				WAKE_LOCK_SUSPEND,
				"vpu_wakelock_1");
	}
#endif
#endif
	wq = create_workqueue("vpu_wq");

err:
	LOG_INF("%s core %d finish\n", __func__, core);
}

void uninit_vpu_power_flag(int core)
{
	kfree(vpu_device);
}

void init_vpu_power_resource(struct device *dev)
{
	int i, j;

	/* define the steps and OPP */
#define DEFINE_APU_STEP(step, m, v0, v1, v2, v3, \
		v4, v5, v6, v7, v8, v9, \
		v10, v11, v12, v13, v14, v15) \
	{ \
		opps.step.index = m - 1; \
		opps.step.count = m; \
		opps.step.values[0] = v0; \
		opps.step.values[1] = v1; \
		opps.step.values[2] = v2; \
		opps.step.values[3] = v3; \
		opps.step.values[4] = v4; \
		opps.step.values[5] = v5; \
		opps.step.values[6] = v6; \
		opps.step.values[7] = v7; \
		opps.step.values[8] = v8; \
		opps.step.values[9] = v9; \
		opps.step.values[10] = v10; \
		opps.step.values[11] = v11; \
		opps.step.values[12] = v12; \
		opps.step.values[13] = v13; \
		opps.step.values[14] = v14; \
		opps.step.values[15] = v15; \
	}

#define DEFINE_APU_OPP(i, v0, v1, v2, v3, v4, v5, v6) \
	{ \
		opps.vvpu.opp_map[i]  = v0; \
		opps.vmdla.opp_map[i]  = v1; \
		opps.dsp.opp_map[i]    = v2; \
		opps.dspcore[0].opp_map[i]       = v3; \
		opps.dspcore[1].opp_map[i]       = v4; \
		opps.ipu_if.opp_map[i] = v5; \
		opps.mdlacore.opp_map[i]    = v6; \
	}

	DEFINE_APU_STEP(vcore, 3, 825000,
			725000, 650000, 0,
			0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0);
	DEFINE_APU_STEP(vvpu, 3, 825000,
			725000, 650000, 0,
			0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0);
	DEFINE_APU_STEP(vmdla, 3, 825000,
			725000, 650000, 0,
			0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0);
	DEFINE_APU_STEP(dsp, 16, 624000,
			624000, 606000, 594000,
			560000, 525000, 450000,
			416000, 364000, 312000,
			273000, 208000, 137000,
			104000, 52000, 26000);
	DEFINE_APU_STEP(dspcore[0], 16, 700000,
			624000, 606000, 594000,
			560000, 525000, 450000,
			416000, 364000, 312000,
			273000, 208000, 137000,
			104000, 52000, 26000);
	DEFINE_APU_STEP(dspcore[1], 16, 700000,
			624000, 606000, 594000,
			560000, 525000, 450000,
			416000, 364000, 312000,
			273000, 208000, 137000,
			104000, 52000, 26000);
	DEFINE_APU_STEP(mdlacore, 16, 788000,
			700000, 624000, 606000,
			594000, 546000, 525000,
			450000, 416000, 364000,
			312000, 273000, 208000,
			137000, 52000, 26000);
	DEFINE_APU_STEP(ipu_if, 16, 624000,
			624000, 606000, 594000,
			560000, 525000, 450000,
			416000, 364000, 312000,
			273000, 208000, 137000,
			104000, 52000, 26000);

	/* default freq */
	DEFINE_APU_OPP(0, 0, 0, 0, 0, 0, 0, 0);
	DEFINE_APU_OPP(1, 0, 0, 1, 1, 1, 1, 1);
	DEFINE_APU_OPP(2, 0, 0, 2, 2, 2, 2, 2);
	DEFINE_APU_OPP(3, 0, 1, 3, 3, 3, 3, 3);
	DEFINE_APU_OPP(4, 0, 1, 4, 4, 4, 4, 4);
	DEFINE_APU_OPP(5, 1, 1, 5, 5, 5, 5, 5);
	DEFINE_APU_OPP(6, 1, 1, 6, 6, 6, 6, 6);
	DEFINE_APU_OPP(7, 1, 1, 7, 7, 7, 7, 7);
	DEFINE_APU_OPP(8, 1, 1, 8, 8, 8, 8, 8);
	DEFINE_APU_OPP(9, 2, 2, 9, 9, 9, 9, 9);
	DEFINE_APU_OPP(10, 2, 2, 10, 10, 10, 10, 10);
	DEFINE_APU_OPP(11, 2, 2, 11, 11, 11, 11, 11);
	DEFINE_APU_OPP(12, 2, 2, 12, 12, 12, 12, 12);
	DEFINE_APU_OPP(13, 2, 2, 13, 13, 13, 13, 13);
	DEFINE_APU_OPP(14, 2, 2, 14, 14, 14, 14, 14);
	DEFINE_APU_OPP(15, 2, 2, 15, 15, 15, 15, 15);

	/* default low opp */
	opps.count = 16;
	opps.index = 4; /* user space usage*/
	opps.vvpu.index = 1;
	opps.dsp.index = 9;
	opps.dspcore[0].index = 9;
	opps.dspcore[1].index = 9;
	opps.ipu_if.index = 9;
	opps.mdlacore.index = 9;
#undef DEFINE_APU_OPP
#undef DEFINE_APU_STEP
	vpu_device->dev[0] = dev;
	vpu_prepare_regulator_and_clock(vpu_device->dev[0], 0);
	vpu_prepare_regulator_and_clock(vpu_device->dev[0], 1);

#ifdef ENABLE_PMQOS
	for (i = 0 ; i < MTK_VPU_CORE ; i++) {
		/* pmqos  */
		pm_qos_add_request(&vpu_qos_vcore_request[i],
				PM_QOS_VCORE_OPP,
				PM_QOS_VCORE_OPP_DEFAULT_VALUE);

		pm_qos_add_request(&vpu_qos_vvpu_request[i],
				PM_QOS_VVPU_OPP,
				PM_QOS_VVPU_OPP_DEFAULT_VALUE);

		pm_qos_add_request(&vpu_qos_vmdla_request[i],
				PM_QOS_VMDLA_OPP,
				PM_QOS_VMDLA_OPP_DEFAULT_VALUE);

		pm_qos_update_request(&vpu_qos_vvpu_request[i],
				VVPU_OPP_2);

		pm_qos_update_request(&vpu_qos_vmdla_request[i],
				VMDLA_OPP_2);
	}

	vmdla_regulator_set_mode(true);
	udelay(100);
	vmdla_regulator_set_mode(false);
	udelay(100);
#endif

	/*init vpu lock power struct*/
	for (i = 0 ; i < VPU_OPP_PRIORIYY_NUM ; i++) {
		for (j = 0 ; j < MTK_VPU_CORE ; j++) {
			lock_power[i][j].core = 0;
			lock_power[i][j].max_boost_value = 100;
			lock_power[i][j].min_boost_value = 0;
			lock_power[i][j].lock = false;
			lock_power[i][j].priority = NORMAL;
			min_opp[j] = VPU_MAX_NUM_OPPS-1;
			max_opp[j] = 0;
			min_boost[j] = 0;
			max_boost[j] = 100;
		}
	}
}

void uninit_vpu_power_resource(void)
{
	int i;

	for (i = 0 ; i < MTK_VPU_CORE ; i++)
		cancel_delayed_work(&(power_counter_work[i].my_work));

	cancel_delayed_work(&opp_keep_work);

#ifdef ENABLE_PMQOS
	for (i = 0 ; i < MTK_VPU_CORE ; i++) {
		pm_qos_remove_request(&vpu_qos_vcore_request[i]);
		pm_qos_remove_request(&vpu_qos_vvpu_request[i]);
		pm_qos_remove_request(&vpu_qos_vmdla_request[i]);
	}
#endif

	vpu_unprepare_regulator_and_clock();

	if (wq) {
		flush_workqueue(wq);
		destroy_workqueue(wq);
		wq = NULL;
	}
}
