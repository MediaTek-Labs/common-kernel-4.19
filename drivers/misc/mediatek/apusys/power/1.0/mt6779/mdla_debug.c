// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "mdla_debug.h"
#include "mdla_dvfs.h"

#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/dcache.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/string.h>

/* global variables */
int g_mdla_log_level = 1;

static int mdla_log_level_set(void *data, u64 val)
{
	g_mdla_log_level = val & 0xf;
	LOG_INF("g_mdla_log_level: %d\n", g_mdla_log_level);

	return 0;
}

static int mdla_log_level_get(void *data, u64 *val)
{
	*val = g_mdla_log_level;

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(mdla_debug_log_level_fops, mdla_log_level_get,
				mdla_log_level_set, "%llu\n");

#define IMPLEMENT_MDLA_DEBUGFS(name)					\
static int mdla_debug_## name ##_show(struct seq_file *s, void *unused)\
{					\
	mdla_dump_## name(s);		\
	return 0;			\
}					\
static int mdla_debug_## name ##_open(struct inode *inode, struct file *file) \
{					\
	return single_open(file, mdla_debug_ ## name ## _show, \
				inode->i_private); \
}                                                                             \
static const struct file_operations mdla_debug_ ## name ## _fops = {   \
	.open = mdla_debug_ ## name ## _open,                               \
	.read = seq_read,                                                    \
	.llseek = seq_lseek,                                                \
	.release = seq_release,                                             \
}


IMPLEMENT_MDLA_DEBUGFS(opp_table);



#undef IMPLEMENT_MDLA_DEBUGFS

static int mdla_debug_power_show(struct seq_file *s, void *unused)
{
	mdla_dump_power(s);
	return 0;
}

static int mdla_debug_power_open(struct inode *inode, struct file *file)
{
	return single_open(file, mdla_debug_power_show, inode->i_private);
}

static ssize_t mdla_debug_power_write(struct file *flip,
		const char __user *buffer,
		size_t count, loff_t *f_pos)
{
	char *tmp, *token, *cursor;
	int ret, i, param;
	const int max_arg = 5;
	unsigned int args[max_arg];

	tmp = kzalloc(count + 1, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	ret = copy_from_user(tmp, buffer, count);
	if (ret) {
		LOG_ERR("copy_from_user failed, ret=%d\n", ret);
		goto out;
	}

	tmp[count] = '\0';

	cursor = tmp;

	/* parse a command */
	token = strsep(&cursor, " ");
	if (strcmp(token, "fix_opp") == 0)
		param = MDLA_POWER_PARAM_FIX_OPP;
	else if (strcmp(token, "dvfs_debug") == 0)
		param = MDLA_POWER_PARAM_DVFS_DEBUG;
	else if (strcmp(token, "jtag") == 0)
		param = MDLA_POWER_PARAM_JTAG;
	else if (strcmp(token, "lock") == 0)
		param = MDLA_POWER_PARAM_LOCK;
	else if (strcmp(token, "volt_step") == 0)
		param = MDLA_POWER_PARAM_VOLT_STEP;
	else if (strcmp(token, "power_hal") == 0)
		param = MDLA_POWER_HAL_CTL;
	else if (strcmp(token, "eara") == 0)
		param = MDLA_EARA_CTL;
	else {
		ret = -EINVAL;
		LOG_ERR("no power param[%s]!\n", token);
		goto out;
	}

	/* parse arguments */
	for (i = 0; i < max_arg && (token = strsep(&cursor, " ")); i++) {
		ret = kstrtouint(token, 10, &args[i]);
		if (ret) {
			LOG_ERR("fail to parse args[%d]\n", i);
			goto out;
		}
	}

	mdla_set_power_parameter(param, i, args);

	ret = count;
out:

	kfree(tmp);
	return ret;
}

static const struct file_operations mdla_debug_power_fops = {
	.open = mdla_debug_power_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
	.write = mdla_debug_power_write,
};

#define DEFINE_MDLA_DEBUGFS(name)  \
	struct dentry *mdla_d##name

	DEFINE_MDLA_DEBUGFS(root);
	DEFINE_MDLA_DEBUGFS(klog);
	DEFINE_MDLA_DEBUGFS(opp_table);
	DEFINE_MDLA_DEBUGFS(power);

u32 mdla_klog;

void mdla_debugfs_init(void)
{
	int ret;

	mdla_klog = 0x40; /* print timeout info by default */

	mdla_droot = debugfs_create_dir("mdla", NULL);

	ret = IS_ERR_OR_NULL(mdla_droot);
	if (ret) {
		LOG_ERR("failed to create debug dir.\n");
		return;
	}

	mdla_dklog = debugfs_create_u32("klog", 0660, mdla_droot,
		&mdla_klog);

#define CREATE_MDLA_DEBUGFS(name)                         \
	{                                                           \
		mdla_d##name = debugfs_create_file(#name, 0644, \
				mdla_droot,         \
				NULL, &mdla_debug_ ## name ## _fops);       \
		if (IS_ERR_OR_NULL(mdla_d##name))                          \
			LOG_ERR("failed to create debug file[" #name "].\n"); \
	}

	CREATE_MDLA_DEBUGFS(opp_table);
	CREATE_MDLA_DEBUGFS(power);

#undef CREATE_MDLA_DEBUGFS
}

void mdla_debugfs_exit(void)
{
#define REMOVE_MDLA_DEBUGFS(name) \
	debugfs_remove(mdla_d##name)

	REMOVE_MDLA_DEBUGFS(klog);
	REMOVE_MDLA_DEBUGFS(opp_table);
	REMOVE_MDLA_DEBUGFS(power);
	REMOVE_MDLA_DEBUGFS(root);
}
