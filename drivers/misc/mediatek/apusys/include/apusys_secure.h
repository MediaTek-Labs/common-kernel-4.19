/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __APUSYS_SECURE_H__
#define __APUSYS_SECURE_H__

#include "apusys_options.h"

#ifdef APUSYS_OPTIONS_SECURE_SUPPORT
#include <mt-plat/mtk_secure_api.h>
#endif

enum MTK_APUSYS_KERNEL_OP {
	MTK_APUSYS_KERNEL_OP_REVISER_SET_BOUNDARY = 0,
	MTK_APUSYS_KERNEL_OP_SET_AO_DBG_SEL,
	MTK_APUSYS_KERNEL_OP_NUM
};

#endif
