// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 * Author: Samuel Hsieh <samuel.hsieh@mediatek.com>
 */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <mtk_battery_percentage_throttling.h>

static unsigned int ut_level;

void bat_per_test(enum BATTERY_PERCENT_LEVEL_TAG level_val)
{
	ut_level = level_val;
	pr_info("[%s] get %d\n", __func__, level_val);
}

static int mtk_pt_bat_per_ut_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "%d\n", ut_level);
	return 0;
}

static ssize_t mtk_pt_bat_per_ut_proc_write
(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	char desc[32];
	int len = 0, ut_input = 0;
	struct power_supply *psy;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
		return 0;
	desc[len] = '\0';

	if (kstrtoint(desc, 10, &ut_input) == 0) {
		if (ut_input == 1 || ut_input == 2) {
			register_battery_percent_notify(&bat_per_test,
				BATTERY_PERCENT_PRIO_UT);
			update_bat_per_ut_status(ut_input);
			psy = power_supply_get_by_name("battery");
			power_supply_changed(psy);
		} else if (ut_input == 0) {
			unregister_battery_percent_notify(
				BATTERY_PERCENT_PRIO_UT);
			update_bat_per_ut_status(ut_input);
		} else {
			pr_info("[%s] wrong number (%d)\n", __func__, ut_input);
		}
	} else
		pr_info("[%s] wrong input (%s)\n", __func__, desc);

	return count;
}

#define PROC_FOPS_RW(name)						\
static int mtk_ ## name ## _proc_open(struct inode *inode, struct file *file)\
{									\
	return single_open(file, mtk_ ## name ## _proc_show, PDE_DATA(inode));\
}									\
static const struct file_operations mtk_ ## name ## _proc_fops = {	\
	.owner		= THIS_MODULE,					\
	.open		= mtk_ ## name ## _proc_open,			\
	.read		= seq_read,					\
	.llseek		= seq_lseek,					\
	.release	= single_release,				\
	.write		= mtk_ ## name ## _proc_write,			\
}

#define PROC_ENTRY(name)	{__stringify(name), &mtk_ ## name ## _proc_fops}

PROC_FOPS_RW(pt_bat_per_ut);

static int battery_percent_create_procfs(void)
{
	struct proc_dir_entry *dir = NULL;
	int i;

	struct pentry {
		const char *name;
		const struct file_operations *fops;
	};

	const struct pentry entries[] = {
		PROC_ENTRY(pt_bat_per_ut),
	};

	dir = proc_mkdir("pt_bat_per", NULL);

	if (!dir) {
		pr_notice("fail to create /proc/pt_bat_per\n");
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(entries); i++) {
		if (!proc_create
		    (entries[i].name, 0664, dir, entries[i].fops))
			pr_notice("@%s: create /proc/pt_bat_per/%s failed\n",
				__func__, entries[i].name);
	}

	return 0;
}

static int __init battery_percentage_throttling_debug_init(void)
{
	battery_percent_create_procfs();
	return 0;
}

static void __exit battery_percentage_throttling_debug_exit(void)
{
}

module_init(battery_percentage_throttling_debug_init);
module_exit(battery_percentage_throttling_debug_exit);

MODULE_AUTHOR("Samuel Hsieh");
MODULE_DESCRIPTION("MTK battery percentage throttling debug");
MODULE_LICENSE("GPL");
