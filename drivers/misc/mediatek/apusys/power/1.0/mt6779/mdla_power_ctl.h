/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _MT6779_MDLA_POWER_CTL_H_
#define _MT6779_MDLA_POWER_CTL_H_

#include <linux/types.h>
#include <linux/printk.h>

struct mdla_power {
	uint8_t boost_value;
	/* align with core index defined in user space header file */
	unsigned int core;
};

enum MDLA_OPP_PRIORIYY {
	MDLA_OPP_DEBUG = 0,
	MDLA_OPP_THERMAL = 1,
	MDLA_OPP_POWER_HAL = 2,
	MDLA_OPP_EARA_QOS = 3,
	MDLA_OPP_NORMAL = 4,
	MDLA_OPP_PRIORIYY_NUM
};

struct mdla_lock_power {
	unsigned int core;
	uint8_t max_boost_value;
	uint8_t min_boost_value;
	bool lock;
	enum MDLA_OPP_PRIORIYY priority;
};

#endif
