/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __APUSYS_SAMPLE_COMMON_H__
#define __APUSYS_SAMPLE_COMMON_H__

#include <linux/printk.h>

#define APUSYS_LOG_PREFIX "[apusys_sample]"

#define smp_drv_err(x, args...) pr_info(APUSYS_LOG_PREFIX\
	"[error] %s " x, __func__, ##args)
#define smp_drv_warn(x, args...) pr_info(APUSYS_LOG_PREFIX\
	"[warn] " x, ##args)
#define smp_drv_info(x, args...) pr_info(APUSYS_LOG_PREFIX\
	x, ##args)
#define smp_drv_debug(x, args...)

#endif
