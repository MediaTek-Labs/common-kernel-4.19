// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>

#include "vpu_dbg.h"
#include "vpu_cmn.h"

#define ALGO_OF_MAX_POWER  (3)

/* global variables */
int g_vpu_log_level = 1;
int g_vpu_internal_log_level;

static int vpu_log_level_set(void *data, u64 val)
{
	g_vpu_log_level = val & 0xf;
	LOG_INF("g_vpu_log_level: %d\n", g_vpu_log_level);

	return 0;
}

static int vpu_log_level_get(void *data, u64 *val)
{
	*val = g_vpu_log_level;

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(vpu_debug_log_level_fops, vpu_log_level_get,
				vpu_log_level_set, "%llu\n");

static int vpu_internal_log_level_set(void *data, u64 val)
{
	g_vpu_internal_log_level = val;
	LOG_INF("g_vpu_internal_log_level: %d\n", g_vpu_internal_log_level);

	return 0;
}

static int vpu_internal_log_level_get(void *data, u64 *val)
{
	*val = g_vpu_internal_log_level;

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(vpu_debug_internal_log_level_fops,
	vpu_internal_log_level_get,
	vpu_internal_log_level_set,
	"%llu\n");

#define IMPLEMENT_VPU_DEBUGFS(name)					\
static int vpu_debug_## name ##_show(struct seq_file *s, void *unused)\
{					\
	vpu_dump_## name(s);		\
	return 0;			\
}					\
static int vpu_debug_## name ##_open(struct inode *inode, struct file *file) \
{					\
	return single_open(file, vpu_debug_ ## name ## _show, \
				inode->i_private); \
}                                                                             \
static const struct file_operations vpu_debug_ ## name ## _fops = {   \
	.open = vpu_debug_ ## name ## _open,                               \
	.read = seq_read,                                                    \
	.llseek = seq_lseek,                                                \
	.release = seq_release,                                             \
}

IMPLEMENT_VPU_DEBUGFS(opp_table);

#undef IMPLEMENT_VPU_DEBUGFS

static int vpu_debug_power_show(struct seq_file *s, void *unused)
{
	vpu_dump_power(s);
	return 0;
}

static int vpu_debug_power_open(struct inode *inode, struct file *file)
{
	return single_open(file, vpu_debug_power_show, inode->i_private);
}

static ssize_t vpu_debug_power_write(struct file *flip,
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
		param = VPU_POWER_PARAM_FIX_OPP;
	else if (strcmp(token, "dvfs_debug") == 0)
		param = VPU_POWER_PARAM_DVFS_DEBUG;
	else if (strcmp(token, "jtag") == 0)
		param = VPU_POWER_PARAM_JTAG;
	else if (strcmp(token, "lock") == 0)
		param = VPU_POWER_PARAM_LOCK;
	else if (strcmp(token, "volt_step") == 0)
		param = VPU_POWER_PARAM_VOLT_STEP;
	else if (strcmp(token, "power_hal") == 0)
		param = VPU_POWER_HAL_CTL;
	else if (strcmp(token, "eara") == 0)
		param = VPU_EARA_CTL;
	else if (strcmp(token, "ct") == 0)
		param = VPU_CT_INFO;
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

	vpu_set_power_parameter(param, i, args);

	ret = count;
out:

	kfree(tmp);
	return ret;
}

static const struct file_operations vpu_debug_power_fops = {
	.open = vpu_debug_power_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
	.write = vpu_debug_power_write,
};

int vpu_init_debug(struct vpu_device *vpu_dev)
{
	int ret;
	struct dentry *debug_file;

	vpu_dev->debug_root = debugfs_create_dir("vpu", NULL);

	ret = IS_ERR_OR_NULL(vpu_dev->debug_root);
	if (ret) {
		LOG_ERR("failed to create debug dir.\n");
		goto out;
	}

#define CREATE_VPU_DEBUGFS(name)                         \
	{                                                           \
		debug_file = debugfs_create_file(#name, 0644, \
				vpu_dev->debug_root,         \
				NULL, &vpu_debug_ ## name ## _fops);       \
		if (IS_ERR_OR_NULL(debug_file))                          \
			LOG_ERR("failed to create debug file[" #name "].\n"); \
	}

	CREATE_VPU_DEBUGFS(log_level);
	CREATE_VPU_DEBUGFS(internal_log_level);
	CREATE_VPU_DEBUGFS(opp_table);
	CREATE_VPU_DEBUGFS(power);

#undef CREATE_VPU_DEBUGFS

out:
	return ret;
}

