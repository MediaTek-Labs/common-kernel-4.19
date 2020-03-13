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

#include "apu_log.h"

extern unsigned int g_mdla_func_mask;
extern void *apu_mdla_cmde_mreg_top;
extern void *apu_mdla_config_top;
extern void *apu_conn_top;

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

#define mdla_dvfs_debug(...)	LOG_DBG(__VA_ARGS__)

#endif

