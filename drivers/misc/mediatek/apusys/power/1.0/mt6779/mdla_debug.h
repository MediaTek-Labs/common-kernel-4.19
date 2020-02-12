/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MDLA_DEBUG_H__
#define __MDLA_DEBUG_H__

#define DEBUG 1

#include <linux/types.h>
#include <linux/printk.h>
#include <linux/seq_file.h>
extern int g_vpu_log_level;
extern unsigned int g_mdla_func_mask;
extern void *apu_mdla_cmde_mreg_top;
extern void *apu_mdla_config_top;
extern void *apu_conn_top;
/* LOG & AEE */
#define MDLA_TAG "[mdla]"
/*#define VPU_DEBUG*/
#ifdef VPU_DEBUG
#define LOG_DBG(format, args...)    pr_debug(MDLA_TAG " " format, ##args)
#else
#define LOG_DBG(format, args...)
#endif
#define LOG_INF(format, args...)    pr_info(MDLA_TAG " " format, ##args)
#define LOG_WRN(format, args...)    pr_info(MDLA_TAG "[warn] " format, ##args)
#define LOG_ERR(format, args...)    pr_info(MDLA_TAG "[error] " format, ##args)

enum MdlaFuncMask {
	VFM_NEED_WAIT_VCORE		= 0x1,
	VFM_ROUTINE_PRT_SYSLOG = 0x2
};

enum MdlaLogThre {
	/* >1, performance break down check */
	MdlaLogThre_PERFORMANCE    = 1,

	/* >2, algo info, opp info check */
	Log_ALGO_OPP_INFO  = 2,

	/* >3, state machine check, while wait vcore/do running */
	Log_STATE_MACHINE  = 3,

	/* >4, dump buffer mva */
	MdlaLogThre_DUMP_BUF_MVA   = 4
};
enum MDLA_power_param {
	MDLA_POWER_PARAM_FIX_OPP,
	MDLA_POWER_PARAM_DVFS_DEBUG,
	MDLA_POWER_PARAM_JTAG,
	MDLA_POWER_PARAM_LOCK,
	MDLA_POWER_PARAM_VOLT_STEP,
	MDLA_POWER_HAL_CTL,
	MDLA_EARA_CTL,
};
#define mdla_print_seq(seq_file, format, args...) \
	{ \
		if (seq_file) \
			seq_printf(seq_file, format, ##args); \
		else \
			LOG_ERR(format, ##args); \
	}


enum MDLA_DEBUG_MASK {
	MDLA_DBG_DRV = 0x01,
	MDLA_DBG_DVFS = 0x80,
};

extern u32 mdla_klog;
#define mdla_debug(mask, ...) do { if (mdla_klog & mask) \
		pr_debug(__VA_ARGS__); \
	} while (0)
void mdla_debugfs_init(void);
void mdla_debugfs_exit(void);

#define mdla_drv_debug(...) mdla_debug(MDLA_DBG_DRV, __VA_ARGS__)
#define mdla_dvfs_debug(...) mdla_debug(MDLA_DBG_DVFS, __VA_ARGS__)

#endif

