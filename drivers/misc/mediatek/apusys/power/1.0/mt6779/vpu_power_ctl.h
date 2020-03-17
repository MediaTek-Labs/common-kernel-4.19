/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _MT6779_VPU_POWER_CTL_H_
#define _MT6779_VPU_POWER_CTL_H_

// CCF do not support mtcmos ctl anymore
#define CCF_MTCMOS_SUPPORT	(0)

// use wakelock of apusys_power_driver to replace this
#define ENABLE_WAKELOCK		(0)

#define MTK_MMDVFS_ENABLE	(0)

#define VPU_MAX_NUM_STEPS               (16)
#define VPU_MAX_NUM_OPPS                (16)

#define OPP_WAIT_TIME_MS    (300)
#define PWR_KEEP_TIME_MS    (0)
#define OPP_KEEP_TIME_MS    (0)
#define SDSP_KEEP_TIME_MS   (5000)
#define POWER_ON_MAGIC          (2)
#define OPPTYPE_VCORE           (0)
#define OPPTYPE_DSPFREQ         (1)
#define OPPTYPE_VVPU            (2)
#define OPPTYPE_VMDLA           (3)

/*
 * Provide two power modes:
 * - dynamic: power-saving mode, it's on request to power on device.
 * - on: power on immediately
 */
enum vpu_power_mode {
	VPU_POWER_MODE_DYNAMIC,
	VPU_POWER_MODE_ON,
};

/*
 * Provide a set of OPPs(operation performance point)
 * The default opp is at the minimun performance,
 * and users could request the performance.
 */
enum vpu_power_opp {
	VPU_POWER_OPP_UNREQUEST = 0xFF,
};

struct vpu_power {
	uint8_t opp_step;
	uint8_t freq_step;
	uint32_t bw; /* unit: MByte/s */

	/* align with core index defined in user space header file */
	unsigned int core;
	uint8_t boost_value;
};


enum VPU_OPP_PRIORIYY {
	DEBUG = 0,
	THERMAL = 1,
	POWER_HAL = 2,
	EARA_QOS = 3,
	NORMAL = 4,
	VPU_OPP_PRIORIYY_NUM
};


struct vpu_lock_power {
	/* align with core index defined in user space header file*/
	unsigned int core;
	uint8_t max_boost_value;
	uint8_t min_boost_value;
	bool lock;
	enum VPU_OPP_PRIORIYY priority;
};

struct vpu_dvfs_steps {
	uint32_t values[VPU_MAX_NUM_STEPS];
	uint8_t count;
	uint8_t index;
	uint8_t opp_map[VPU_MAX_NUM_OPPS];
};

struct vpu_dvfs_opps {
	struct vpu_dvfs_steps vcore;
	struct vpu_dvfs_steps vvpu;
	struct vpu_dvfs_steps vmdla;
	struct vpu_dvfs_steps dsp;      /* ipu_conn */
	struct vpu_dvfs_steps dspcore[MTK_VPU_CORE];    /* ipu_core# */
	struct vpu_dvfs_steps mdlacore; /* ipu_core# */
	struct vpu_dvfs_steps ipu_if;   /* ipusys_vcore, interface */
	uint8_t index;
	uint8_t count;
};

enum VpuPowerOnType {
	/* power on previously by setPower */
	VPT_PRE_ON              = 1,

	/* power on by enque */
	VPT_ENQUE_ON    = 2,

	/* power on by enque, but want to immediately off(when exception) */
	VPT_IMT_OFF             = 3,

	/* same as VPT_IMT_OFF, from secure DSP */
	VPT_SDSP_OFF    = 4,
};

void vpu_opp_keep_routine(struct work_struct *);
void vpu_sdsp_routine(struct work_struct *work);
int vpu_boot_up(int core, bool secure);
int vpu_shut_down(int core);
int vpu_lock_set_power(struct vpu_lock_power *vpu_lock_power);

extern struct vpu_device *vpu_device;
extern void init_vpu_power_flag(int core);
extern void init_vpu_power_resource(struct device *dev);
extern void uninit_vpu_power_resource(void);

extern void vpu_opp_check(int core, uint8_t vvpu_index, uint8_t freq_index);
extern int vpu_get_power(int core, bool secure);
extern void vpu_put_power(int core, enum VpuPowerOnType type);
extern void vpu_put_power_nowq(int core);
extern uint8_t vpu_boost_value_to_opp(uint8_t boost_value);
extern void vpu_opp_mapping_check(int core, int vpu_opp);
#endif
