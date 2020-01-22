/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __APUSYS_DEBUG_H__
#define __APUSYS_DEBUG_H__

#define APUSYS_DBG_DIR "midware"

enum {
	DBG_PROP_MULTICORE,
	DBG_PROP_TCM_DEFAULT,
	DBG_PROP_QUERY_MEM,

	DBG_PROP_MAX,
};

extern bool apusys_dump_force;
extern bool apusys_dump_skip;

int dbg_get_prop(int idx);

int apusys_dbg_init(struct dentry *root);
int apusys_dbg_destroy(void);

void apusys_dump_init(void);
void apusys_reg_dump(void);
void apusys_dump_exit(void);
int apusys_dump_show(struct seq_file *sfile, void *v);
void apusys_dump_reg_skip(int onoff);

#endif
