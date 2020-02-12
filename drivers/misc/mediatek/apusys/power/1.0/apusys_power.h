/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _APUSYS_POWER_H_
#define _APUSYS_POWER_H_

/******************************************************
 * for apusys power platform device API
 ******************************************************/

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
	VPU2 = 2,
	MDLA0 = 3,
	MDLA1 = 4,
	APUSYS_DVFS_USER_NUM,

	EDMA = 0x10,    // power user only
	EDMA2 = 0x11,   // power user only
	REVISER = 0x12, // power user only
	APUSYS_POWER_USER_NUM,
};

extern int apu_power_device_register(enum DVFS_USER,
				struct platform_device *pdev);
extern void apu_power_device_unregister(enum DVFS_USER);
extern int apu_device_power_on(enum DVFS_USER);
extern int apu_device_power_off(enum DVFS_USER);
extern int apu_device_power_suspend(enum DVFS_USER user, int suspend);
extern void apu_device_set_opp(enum DVFS_USER user, uint8_t opp);
extern uint64_t apu_get_power_info(uint8_t force);
extern bool apu_get_power_on_status(enum DVFS_USER user);
extern void apu_power_on_callback(void);
extern int apu_power_callback_device_register(enum POWER_CALLBACK_USER user,
					void (*power_on_callback)(void *para),
					void (*power_off_callback)(void *para));
extern void apu_power_callback_device_unregister(enum POWER_CALLBACK_USER user);
extern uint8_t apusys_boost_value_to_opp
				(enum DVFS_USER user, uint8_t boost_value);
extern enum DVFS_FREQ apusys_opp_to_freq(enum DVFS_USER user, uint8_t opp);
extern int8_t apusys_get_ceiling_opp(enum DVFS_USER user);
extern int8_t apusys_get_opp(enum DVFS_USER user);
extern void apu_power_reg_dump(void);
extern int apu_power_power_stress(int type, int device, int opp);
extern bool apusys_power_check(void);
extern void apu_set_vcore_boost(bool enable);


#endif
