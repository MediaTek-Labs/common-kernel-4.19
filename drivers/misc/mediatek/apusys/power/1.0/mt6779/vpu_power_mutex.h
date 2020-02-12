/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _MT6779_VPU_POWER_MUTEX_H_
#define _MT6779_VPU_POWER_MUTEX_H_

extern struct mutex power_mutex[MTK_VPU_CORE];
extern struct mutex power_counter_mutex[MTK_VPU_CORE];
extern struct mutex opp_mutex;
extern struct mutex power_lock_mutex;

#endif
