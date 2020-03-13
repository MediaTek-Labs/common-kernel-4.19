/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _APUSYS_POWER_USER_H_
#define _APUSYS_POWER_USER_H_

/******************************************************
 * for apusys power platform device API
 ******************************************************/

#define DEBUG_OPP_TEST		(0)
#define PRE_POWER_ON		(0)
#define APUSYS_MAX_NUM_OPPS	(16)
#define	SUPPORT_MDLA		(1)
#define BRING_UP		(1)

// unsed in power 1.0
enum POWER_CALLBACK_USER {
	IOMMU = 0,
	REVISOR = 1,
	MNOC = 2,
	DEVAPC = 3,
	APUSYS_POWER_CALLBACK_USER_NUM,
};

enum DVFS_USER {
	VPU0 = 0,
	VPU1 = 1,
	MDLA0 = 2,
	APUSYS_DVFS_USER_NUM,

	EDMA = 0x10,    // power user only, unsed in power 1.0
	EDMA2 = 0x11,   // power user only, unsed in power 1.0
	REVISER = 0x12, // power user only, unsed in power 1.0
	APUSYS_POWER_USER_NUM,
};

#endif // _APUSYS_POWER_USER_H_
