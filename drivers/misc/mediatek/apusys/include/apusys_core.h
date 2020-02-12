/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __APUSYS_CORE_H__
#define __APUSYS_CORE_H__

struct apusys_core_info {
	struct dentry *dbg_root;
};

/* declare init/exit func at other module */
int midware_init(struct apusys_core_info *info);
void midware_exit(void);
int sample_init(struct apusys_core_info *info);
void debug_exit(void);
int debug_init(struct apusys_core_info *info);
void sample_exit(void);

int EDMA_INIT(struct apusys_core_info *info);
void EDMA_EXIT(void);

/*
 * init function at other modulses
 * call init function in order at apusys.ko init stage
 */
static int (*apusys_init_func[])(struct apusys_core_info *) = {
	midware_init,
	sample_init,
	debug_init,
	EDMA_INIT,
};

/*
 * exit function at other modulses
 * call exit function in order at apusys.ko exit stage
 */
static void (*apusys_exit_func[])(void) = {
	sample_exit,
	midware_exit,
	debug_exit,
	EDMA_EXIT,
};

#endif
