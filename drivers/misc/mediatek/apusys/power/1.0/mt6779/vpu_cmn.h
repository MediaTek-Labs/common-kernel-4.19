/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __VPU_CMN_H__
#define __VPU_CMN_H__

#include <linux/wait.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>
#include "vpu_dbg.h"

#ifdef CONFIG_MTK_AEE_FEATURE
#include <aee.h>
#endif

struct vpu_device {
	struct proc_dir_entry *proc_dir;
	struct device *dev[1]; // apu_power device
	struct dentry *debug_root;
};

struct vpu_user {
	pid_t open_pid;
	pid_t open_tgid;
	uint8_t power_mode;
	uint8_t power_opp;
};

enum vpu_power_param {
	VPU_POWER_PARAM_FIX_OPP,
	VPU_POWER_PARAM_DVFS_DEBUG,
	VPU_POWER_PARAM_JTAG,
	VPU_POWER_PARAM_LOCK,
	VPU_POWER_PARAM_VOLT_STEP,
	VPU_POWER_HAL_CTL,
	VPU_EARA_CTL,
	VPU_CT_INFO,
	VPU_POWER_PARAM_DISABLE_OFF,
};


/* enum & structure */
enum VpuCoreState {
	VCT_SHUTDOWN    = 1,
	VCT_BOOTUP              = 2,
	VCT_EXECUTING   = 3,
	VCT_IDLE                = 4,
	VCT_VCORE_CHG   = 5,
	VCT_NONE                = -1,
};



/**
 * vpu_dump_opp_table - dump the OPP table
 * @s:          the pointer to seq_file.
 */
int vpu_dump_opp_table(struct seq_file *s);

/**
 * vpu_dump_power - dump the power parameters
 * @s:          the pointer to seq_file.
 */
int vpu_dump_power(struct seq_file *s);

/**
 * vpu_set_power_parameter - set the specific power parameter
 * @param:      the sepcific parameter to update
 * @argc:       the number of arguments
 * @args:       the pointer of arryf of arguments
 */
int vpu_set_power_parameter(uint8_t param, int argc, int *args);

/* LOG & AEE */
#define VPU_TAG "[vpu]"
#define VPU_DEBUG
#ifdef VPU_DEBUG
#define LOG_DBG(format, args...)    pr_debug(VPU_TAG " " format, ##args)
#else
#define LOG_DBG(format, args...)
#endif

#define LOG_DVFS(format, args...) \
	do { if (g_vpu_log_level > Log_STATE_MACHINE) \
		pr_info(VPU_TAG " " format, ##args); \
	} while (0)
#define LOG_INF(format, args...)    pr_debug(VPU_TAG " " format, ##args)
#define LOG_WRN(format, args...)    pr_debug(VPU_TAG "[warn] " format, ##args)
#define LOG_ERR(format, args...)    pr_info(VPU_TAG "[error] " format, ##args)

#define PRINT_LINE() pr_info(VPU_TAG " %s (%s:%d)\n", \
		__func__,  __FILE__, __LINE__)

#define vpu_print_seq(seq_file, format, args...) \
{ \
	if (seq_file) \
		seq_printf(seq_file, format, ##args); \
	else \
		LOG_ERR(format, ##args); \
}

#ifdef CONFIG_MTK_AEE_FEATURE
#define vpu_aee(key, format, args...) \
	do { \
		LOG_ERR(format, ##args); \
		aee_kernel_exception("VPU", \
				"\nCRDISPATCH_KEY:" key "\n" format, ##args); \
	} while (0)

#define vpu_aee_warn(key, format, args...) \
	do { \
		LOG_ERR(format, ##args); \
		aee_kernel_warning("VPU", \
				"\nCRDISPATCH_KEY:" key "\n" format, ##args); \
	} while (0)
#else
#define vpu_aee(key, format, args...)
#define vpu_aee_warn(key, format, args...)
#endif

#endif
