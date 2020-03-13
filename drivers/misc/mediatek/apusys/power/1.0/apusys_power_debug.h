/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _APUSYS_POWER_DEBUG_H_
#define _APUSYS_POWER_DEBUG_H_

enum APUSYS_POWER_PARAM {
	POWER_PARAM_FIX_OPP,
	POWER_PARAM_DVFS_DEBUG,
	POWER_HAL_CTL,
	POWER_PARAM_POWER_STRESS,
	POWER_PARAM_OPP_TABLE,
	POWER_PARAM_CURR_STATUS,
	POWER_PARAM_LOG_LEVEL,
};

void apusys_power_debugfs_init(void);
void apusys_power_debugfs_exit(void);

#endif
