/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __VPU_DBG_H__
#define __VPU_DBG_H__

extern int g_vpu_log_level;
extern int g_vpu_internal_log_level;

enum VpuLogThre {
	/* >1, performance break down check */
	VpuLogThre_PERFORMANCE    = 1,

	/* >2, algo info, opp info check */
	Log_ALGO_OPP_INFO  = 2,

	/* >3, state machine check, while wait vcore/do running */
	Log_STATE_MACHINE  = 3,

	/* >4, dump buffer mva */
	VpuLogThre_DUMP_BUF_MVA   = 4
};

#endif
