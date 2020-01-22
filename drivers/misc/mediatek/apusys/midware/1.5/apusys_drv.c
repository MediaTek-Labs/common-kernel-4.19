// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/module.h>  /* Needed by all modules */
#include <linux/kernel.h>  /* Needed for KERN_ALERT */
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#include <linux/init.h>

#include "apusys_options.h"
#ifdef APUSYS_OPTIONS_INF_POWERARCH
#include "apusys_power.h"
#endif
#include "apusys_core.h"
#include "apusys_drv.h"
#include "apusys_user.h"
#include "apusys_cmn.h"
#include "scheduler.h"
#include "resource_mgt.h"
#include "memory_mgt.h"
#include "cmd_parser.h"
#include "apusys_dbg.h"

#include "apusys_device.h"
#include "secure_perf.h"


#include <linux/dma-mapping.h>

/* define */
#define APUSYS_DEV_NAME "apusys_midware"
#define APUSYS_DEV_NODE_NAME "apusys"

/* global variable */
static dev_t midware_devt;
static struct cdev *midware_cdev;
static struct class *midware_class;
//static struct device *midware_dev;
static struct device *g_midware_device;
static struct apusys_core_info *g_core_info;


/* function declaration */
static int apusys_open(struct inode *, struct file *);
static int apusys_release(struct inode *, struct file *);
static long apusys_ioctl(struct file *, unsigned int, unsigned long);
static long apusys_compat_ioctl(struct file *, unsigned int, unsigned long);
static int apusys_mmap(struct file *filp, struct vm_area_struct *vma);

static const struct file_operations apusys_fops = {
	.open = apusys_open,
	.unlocked_ioctl = apusys_ioctl,
	.compat_ioctl = apusys_compat_ioctl,
	.release = apusys_release,
	.mmap = apusys_mmap,
};

static int apusys_mmap(struct file *filp, struct vm_area_struct *vma)
{
#ifdef APUSYS_OPTIONS_MEM_DMA
	unsigned long offset = vma->vm_pgoff;
	unsigned long size = vma->vm_end - vma->vm_start;

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
	if (remap_pfn_range(vma, vma->vm_start, offset, size,
			vma->vm_page_prot)) {
		mdw_drv_err("remap_pfn_range fail: %p\n", vma);
		return -EAGAIN;
	}

	return 0;
#else
	mdw_drv_err("not support mmap(%p)(%lu/%lu)\n",
		filp, vma->vm_start, vma->vm_end);
	return -EINVAL;
#endif
}

static int apusys_open(struct inode *inode, struct file *filp)
{
	struct apusys_user *u = NULL;
	int ret = 0;

	ret = apusys_create_user(&u);
	if (ret) {
		mdw_drv_err("create apusys user fail (%d)\n", ret);
		return -ENOMEM;
	}

	filp->private_data = u;

	mdw_flw_debug("create user (0x%llx/%d/%d)\n",
		u->id,
		u->open_pid,
		u->open_tgid);

	return 0;
}

static int apusys_release(struct inode *inode, struct file *filp)
{
	struct apusys_user *u = filp->private_data;
	int ret = 0;

	if (u == NULL) {
		mdw_drv_err("miss apusys user\n");
		return -ENOMEM;
	}

	mdw_flw_debug("delete user %p(0x%llx/%d/%d)\n",
		u,
		u->id,
		(int)u->open_pid,
		(int)u->open_tgid);

	ret = apusys_delete_user(u);
	if (ret) {
		mdw_drv_err("delete apusys user fail(%d)\n", ret);
		return -EINVAL;
	}

	return 0;
}

static int apusys_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = NULL;

	mdw_drv_debug("+\n");

	g_midware_device = &pdev->dev;

	/* get major */
	ret = alloc_chrdev_region(&midware_devt, 0, 1, APUSYS_DEV_NODE_NAME);
	if (ret < 0) {
		mdw_drv_err("alloc_chrdev_region failed, %d\n", ret);
		return ret;
	}

	/* Allocate driver */
	midware_cdev = cdev_alloc();
	if (midware_cdev == NULL) {
		mdw_drv_err("cdev_alloc failed\n");
		ret = -ENOMEM;
		goto out;
	}

	/* Attatch file operation. */
	cdev_init(midware_cdev, &apusys_fops);
	midware_cdev->owner = THIS_MODULE;

	/* Add to system */
	ret = cdev_add(midware_cdev, midware_devt, 1);
	if (ret < 0) {
		mdw_drv_err("attatch file operation failed, %d\n", ret);
		goto out;
	}

	/* Create class register */
	midware_class = class_create(THIS_MODULE, APUSYS_DEV_NAME);
	if (IS_ERR(midware_class)) {
		ret = PTR_ERR(midware_class);
		mdw_drv_err("unable to create class, err = %d\n", ret);
		goto out;
	}

	dev = device_create(midware_class, NULL, midware_devt,
		NULL, APUSYS_DEV_NODE_NAME);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		mdw_drv_err("failed to create device: /dev/%s, err = %d",
			APUSYS_DEV_NODE_NAME, ret);
		goto out;
	}

	/* init function */
	if (apusys_user_init()) {
		ret = -EINVAL;
		goto out;
	}
	if (res_mgt_init()) {
		ret = -EINVAL;
		goto resource_init_fail;
	}
	if (apusys_sched_init()) {
		ret = -EINVAL;
		goto sched_init_fail;
	}
	if (apusys_mem_init(&pdev->dev)) {
		ret = -EINVAL;
		goto mem_init_fail;
	}
	if (apusys_dbg_init(g_core_info->dbg_root)) {
		ret = -EINVAL;
		goto dbg_init_fail;
	}

	secure_perf_init();

	mdw_drv_debug("-\n");

	return 0;

dbg_init_fail:
	apusys_mem_destroy();
mem_init_fail:
	apusys_sched_destroy();
sched_init_fail:
	res_mgt_destroy();
resource_init_fail:
	apusys_user_destroy();
out:

	/* Release device */
	if (dev != NULL)
		device_destroy(midware_class, midware_devt);

	/* Release class */
	if (midware_class != NULL) {
		class_destroy(midware_class);
		midware_class = NULL;
	}

	/* Release char driver */
	if (midware_cdev != NULL) {
		cdev_del(midware_cdev);
		midware_cdev = NULL;
	}

	unregister_chrdev_region(midware_devt, 1);

	return ret;
}

static int apusys_remove(struct platform_device *pdev)
{
	mdw_flw_debug("+\n");
	/* release logical resource */
	apusys_dbg_destroy();
	apusys_mem_destroy();
	apusys_sched_destroy();
	res_mgt_destroy();
	apusys_user_destroy();

	/* Release device */
	device_destroy(midware_class, midware_devt);

	/* Release class */
	if (midware_class != NULL) {
		class_destroy(midware_class);
		midware_class = NULL;
	}

	/* Release char driver */
	if (midware_cdev != NULL) {
		cdev_del(midware_cdev);
		midware_cdev = NULL;
	}

	unregister_chrdev_region(midware_devt, 1);

	secure_perf_remove();
	mdw_flw_debug("-\n");
	return 0;
}

static int apusys_suspend(struct platform_device *pdev, pm_message_t mesg)
{
#ifdef APUSYS_OPTIONS_SUSPEND_SUPPORT
	apusys_sched_pause();
#endif
	return 0;
}

static int apusys_resume(struct platform_device *pdev)
{
#ifdef APUSYS_OPTIONS_SUSPEND_SUPPORT
	apusys_sched_restart();
#endif
	return 0;
}

static long apusys_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0, count = 0, dev_num = 0, val = 0;
	struct apusys_user *user = (struct apusys_user *)filp->private_data;
	struct apusys_mem mem;
	struct apusys_kmem kmem;
	struct apusys_ioctl_cmd ioctl_cmd;
	struct apusys_ioctl_hs hs;
	//struct apusys_ioctl_dev dev_alloc;
	struct apusys_ioctl_power ioctl_pwr;
	struct apusys_ioctl_fw ioctl_fw;
	struct apusys_ioctl_sec ioctl_sec;
	struct apusys_ioctl_ucmd ioctl_ucmd;
	struct apusys_cmd *a_cmd = NULL;
	struct apusys_dev_aquire acq;
	struct list_head *tmp = NULL, *list_ptr = NULL;
	struct apusys_dev_info *dev_info = NULL;

	/* check apusys user is available */
	if (user == NULL) {
		mdw_drv_err("ioctl miss apusys user");
		return -EINVAL;
	}

	switch (cmd) {
	case APUSYS_IOCTL_HANDSHAKE:
		mdw_lne_debug();
		/* handshaking, pass kernel apusys information */
		if (copy_from_user(&hs, (void *)arg,
			sizeof(struct apusys_ioctl_hs))) {
			mdw_drv_err("copy handshake struct fail\n");
			ret = -EINVAL;
			goto out;
		}

		hs.magic_num = get_cmdformat_magic();
		hs.cmd_version = get_cmdformat_version();
		switch (hs.type) {
		case APUSYS_HANDSHAKE_BEGIN:
			hs.begin.mem_support = apusys_mem_get_support();
			hs.begin.dev_support = res_get_dev_support();
			hs.begin.dev_type_max = APUSYS_DEVICE_MAX;
			if (hs.begin.mem_support & (1UL << APUSYS_MEM_VLM)) {
				if (apusys_mem_get_vlm(&hs.begin.vlm_start,
					&hs.begin.vlm_size)) {
					mdw_drv_warn("vlm miss start and size\n");
				}
			} else {
				hs.begin.vlm_start = 0;
				hs.begin.vlm_size = 0;
			}

			mdw_flw_debug("support dev(0x%llx)mem(0x%x/0x%x/%u)\n",
				hs.begin.dev_support,
				hs.begin.mem_support,
				hs.begin.vlm_start,
				hs.begin.vlm_size);
			break;

		case APUSYS_HANDSHAKE_QUERY_DEV:
			hs.dev.num = res_get_device_num(hs.dev.type);
			mdw_flw_debug("device(%d) support(%d) core\n",
				hs.dev.type, hs.dev.num);

			if (hs.dev.num < 0)
				ret = -EINVAL;

			break;

		default:
			mdw_flw_debug("wrong handshake type(%d)\n",
				hs.type);
			ret = -EINVAL;
			break;
		}

		if (copy_to_user((void *)arg, &hs,
			sizeof(struct apusys_ioctl_hs))) {
			mdw_drv_err("handshake with user fail\n");
			ret = -EINVAL;
		}
		break;

	case APUSYS_IOCTL_MEM_ALLOC:
		mdw_lne_debug();
		if (copy_from_user(&mem, (void *)arg,
			sizeof(struct apusys_mem))) {
			mdw_drv_err("copy mem struct fail\n");
			ret = -EINVAL;
			goto out;
		}

		apusys_mem_copy_from_user(&mem, &kmem);

		/* allocate memory, mapping kernel va */
		ret = apusys_mem_alloc(&kmem);
		if (ret)
			break;

		apusys_mem_copy_to_user(&mem, &kmem);

		if (copy_to_user((void *)arg, &mem,
			sizeof(struct apusys_mem))) {
			mdw_drv_err("copy mem struct to user fail\n");
			if (apusys_mem_free(&kmem))
				mdw_drv_err("free unused mem fail\n");

			ret = -EINVAL;
		} else {
			if (apusys_user_insert_mem(user, &kmem)) {
				mdw_drv_err("insmemd fail(0x%llx/%d/%d/0x%llx)\n",
					user->id,
					kmem.mem_type,
					kmem.fd,
					kmem.kva);
				ret = -EINVAL;

				if (apusys_mem_free(&kmem))
					mdw_drv_err("free unused mem fail\n");
			}
		}

		break;

	case APUSYS_IOCTL_MEM_FREE:
		mdw_lne_debug();
		if (copy_from_user(&mem, (void *)arg,
			sizeof(struct apusys_mem))) {
			mdw_drv_err("copy mem struct fail\n");
			ret = -EINVAL;
			goto out;
		}

		apusys_mem_copy_from_user(&mem, &kmem);

		/* free memory, free kernel memory ref count */
		ret = apusys_mem_free(&kmem);
		if (ret) {
			mdw_drv_err("free apusys mem fail\n");
			break;
		}

		apusys_mem_copy_to_user(&mem, &kmem);

		if (apusys_user_delete_mem(user, &kmem)) {
			mdw_drv_err("delmem fail(0x%llx/%d/%d/0x%llx)\n",
				user->id,
				kmem.mem_type,
				kmem.fd,
				kmem.kva);
			ret = -EINVAL;
			goto out;
		} else {
			if (copy_to_user((void *)arg, &mem,
				sizeof(struct apusys_mem))) {

				mdw_drv_err("copy mem struct to user fail\n");
				ret = -EINVAL;
			}
		}

		break;
	case APUSYS_IOCTL_MEM_IMPORT:
		mdw_lne_debug();
		if (copy_from_user(&mem, (void *)arg,
			sizeof(struct apusys_mem))) {
			mdw_drv_err("copy mem struct fail\n");
			ret = -EINVAL;
			goto out;
		}

		apusys_mem_copy_from_user(&mem, &kmem);

		/* allocate memory, mapping kernel va */
		ret = apusys_mem_import(&kmem);
		if (ret)
			break;

		apusys_mem_copy_to_user(&mem, &kmem);

		if (copy_to_user((void *)arg, &mem,
			sizeof(struct apusys_mem))) {
			mdw_drv_err("copy mem struct to user fail\n");
			if (apusys_mem_unimport(&kmem))
				mdw_drv_err("free unused mem fail\n");

			ret = -EINVAL;
		} else {
			if (apusys_user_insert_mem(user, &kmem)) {
				mdw_drv_err("insmemd fail(0x%llx/%d/%d/0x%llx)\n",
					user->id,
					kmem.mem_type,
					kmem.fd,
					kmem.kva);
				ret = -EINVAL;

				if (apusys_mem_unimport(&kmem))
					mdw_drv_err("free unused mem fail\n");
			}
		}

		break;

	case APUSYS_IOCTL_MEM_UNIMPORT:
		mdw_lne_debug();
		if (copy_from_user(&mem, (void *)arg,
			sizeof(struct apusys_mem))) {
			mdw_drv_err("copy mem struct fail\n");
			ret = -EINVAL;
			goto out;
		}

		memset(&kmem, 0, sizeof(struct apusys_kmem));
		apusys_mem_copy_from_user(&mem, &kmem);

		/* free memory, free kernel memory ref count */
		ret = apusys_mem_unimport(&kmem);
		if (ret) {
			mdw_drv_err("free apusys mem fail\n");
			break;
		}

		apusys_mem_copy_to_user(&mem, &kmem);

		if (apusys_user_delete_mem(user, &kmem)) {
			mdw_drv_err("delmem fail(0x%llx/%d/%d/0x%llx)\n",
				user->id,
				kmem.mem_type,
				kmem.fd,
				kmem.kva);
			ret = -EINVAL;
			goto out;
		} else {
			if (copy_to_user((void *)arg, &mem,
				sizeof(struct apusys_mem))) {

				mdw_drv_err("copy mem struct to user fail\n");
				ret = -EINVAL;
			}
		}

		break;
	case APUSYS_IOCTL_MEM_CTL:
		mdw_lne_debug();
		if (copy_from_user(&mem, (void *)arg,
			sizeof(struct apusys_mem))) {
			mdw_drv_err("copy mem struct fail\n");
			ret = -EINVAL;
			goto out;
		}
		apusys_mem_copy_from_user(&mem, &kmem);

		ret = apusys_mem_ctl(&mem.ctl_data, &kmem);
		if (ret)
			mdw_drv_err("control apusys mem fail\n");

		apusys_mem_copy_to_user(&mem, &kmem);

		if (ret == 0) {
			if (copy_to_user((void *)arg, &mem,
				sizeof(struct apusys_mem))) {
				mdw_drv_err("copy mem struct to user fail\n");
				ret = -EINVAL;
			}
		}
		break;

	case APUSYS_IOCTL_RUN_CMD_SYNC:
		if (copy_from_user(&ioctl_cmd, (void *)arg,
			sizeof(struct apusys_ioctl_cmd))) {
			mdw_drv_err("copy cmd struct fail\n");
			ret = -EINVAL;
			goto out;
		}

		/* parse command buffer, and get struct apusys_cmd */
		ret = apusys_cmd_create(ioctl_cmd.mem_fd, ioctl_cmd.offset,
			&a_cmd, user);
		if (ret || a_cmd == NULL) {
			mdw_drv_err("parser cmd fail(%d/%p).\n", ret, a_cmd);
			ret = -EINVAL;
			goto out;
		}

		/* insert cmd to user list */
		ret = apusys_user_insert_cmd(user, (void *)a_cmd);
		if (ret) {
			mdw_drv_err("insert apusys cmd(0x%llx) to user list fail\n",
				a_cmd->cmd_id);
			if (apusys_cmd_delete(a_cmd))
				mdw_drv_err("delete apusys cmd(%p) fail\n",
					a_cmd);

			goto out;
		}

		/* insert cmd to priority queue */
		if (apusys_sched_add_cmd(a_cmd)) {
			mdw_drv_err("add cmd(0x%llx) to list fail\n",
				a_cmd->cmd_id);
			ret = -EINVAL;
			goto run_sync_add_fail;
		}

		/* wait cmd done */
		ret = apusys_sched_wait_cmd(a_cmd);
		if (ret) {
			mdw_drv_err("cmd(0x%llx) wait fail(%d)\n",
				a_cmd->cmd_id, ret);
		} else {
			ret = a_cmd->cmd_ret;
			if (ret) {
				mdw_drv_err("cmd(0x%llx) exec fail(%d)\n",
					a_cmd->cmd_id, ret);
			}
		}

run_sync_add_fail:
		/* delete cmd from user list */
		if (apusys_user_delete_cmd(user, (void *)a_cmd)) {
			mdw_drv_err("delete cmd(0x%llx) from user fail\n",
			a_cmd->cmd_id);
			ret = -EINVAL;
		}

		/* delete cmd */
		if (apusys_cmd_delete(a_cmd)) {
			mdw_drv_err("delete apusys cmd(%p) fail\n", a_cmd);
			ret = -EINVAL;
		}

		break;

	case APUSYS_IOCTL_RUN_CMD_ASYNC:
		if (copy_from_user(&ioctl_cmd, (void *)arg,
			sizeof(struct apusys_ioctl_cmd))) {
			mdw_drv_err("copy cmd struct fail\n");
			ret = -EINVAL;
			goto out;
		}

		/* parse command buffer, and get struct apusys_cmd */
		ret = apusys_cmd_create(ioctl_cmd.mem_fd, ioctl_cmd.offset,
			&a_cmd, user);
		if (ret || a_cmd == NULL) {
			mdw_drv_err("parser cmd fail(%d/%p).\n", ret, a_cmd);
			ret = -EINVAL;
			goto out;
		}

		/* insert cmd to user list */
		ret = apusys_user_insert_cmd(user, (void *)a_cmd);
		if (ret) {
			mdw_drv_err("insert cmd(0x%llx) to user list fail\n",
				a_cmd->cmd_id);

			if (apusys_cmd_delete(a_cmd))
				mdw_drv_err("delete cmd(%p) fail\n", a_cmd);

			goto out;
		}

		/* insert cmd to priority queue */
		ret = apusys_sched_add_cmd(a_cmd);
		if (ret) {
			mdw_drv_err("add cmd(0x%llx) to list fail\n",
				a_cmd->cmd_id);
			if (apusys_user_delete_cmd(user, a_cmd))
				mdw_drv_err("delete cmd(0x%llx) from user fail\n",
				a_cmd->cmd_id);

			if (apusys_cmd_delete(a_cmd))
				mdw_drv_err("delete cmd(%p) fail\n", a_cmd);

			goto out;
		}

		ioctl_cmd.cmd_id = a_cmd->cmd_id;
		if (copy_to_user((void *)arg, &ioctl_cmd,
			sizeof(struct apusys_ioctl_cmd))) {
			mdw_drv_err("copy cmd struct to user fail\n");
			ret = -EINVAL;
		}

		break;

	case APUSYS_IOCTL_WAIT_CMD:
		mdw_lne_debug();
		if (copy_from_user(&ioctl_cmd, (void *)arg,
			sizeof(struct apusys_ioctl_cmd))) {
			mdw_drv_err("copy cmd struct fail\n");
			ret = -EINVAL;
			goto out;
		}

		/* check cmd id */
		if (ioctl_cmd.cmd_id == 0) {
			ret = -EINVAL;
			goto out;
		}

		/* get cmd from user list */
		ret = apusys_user_get_cmd(user, (void **)&a_cmd,
		ioctl_cmd.cmd_id);
		if (ret) {
			mdw_drv_err("get cmd(0x%llx) from user fail\n",
				ioctl_cmd.cmd_id);
			goto out;
		}

		/* wait cmd done */
		ret = apusys_sched_wait_cmd(a_cmd);
		if (ret) {
			mdw_drv_err("cmd(0x%llx) wait fail(%d)\n",
				a_cmd->cmd_id, ret);
		} else {
			ret = a_cmd->cmd_ret;
			if (ret) {
				mdw_drv_err("cmd(0x%llx) exec fail(%d)\n",
					a_cmd->cmd_id, ret);
			}
		}

		/* delete cmd from user list */
		if (apusys_user_delete_cmd(user, (void *)a_cmd)) {
			mdw_drv_err("delete cmd(0x%llx) from user fail\n",
			a_cmd->cmd_id);
			ret = -EINVAL;
		}

		/* delete cmd */
		if (apusys_cmd_delete(a_cmd)) {
			mdw_drv_err("delete cmd(%p) fail\n", a_cmd);
			ret = -EINVAL;
		}

		break;

	case APUSYS_IOCTL_SET_POWER:
		mdw_lne_debug();
		if (copy_from_user(&ioctl_pwr, (void *)arg,
			sizeof(struct apusys_ioctl_power))) {
			mdw_drv_err("copy power struct fail\n");
			ret = -EINVAL;
			goto out;
		}

		mdw_flw_debug("set power: device(%d/%d) boost_val(%d)\n",
			ioctl_pwr.dev_type,
			ioctl_pwr.idx,
			ioctl_pwr.boost_val);

		/* call power on functions */
		ret = res_power_on(ioctl_pwr.dev_type, ioctl_pwr.idx,
			ioctl_pwr.boost_val, APUSYS_SETPOWER_TIMEOUT);
		if (ret) {
			mdw_drv_err("set power: device(%d/%u/%u) (%d)",
				ioctl_pwr.dev_type,
				ioctl_pwr.idx,
				ioctl_pwr.boost_val,
				ret);
		}

		break;

	case APUSYS_IOCTL_DEVICE_ALLOC:
		mdw_lne_debug();
		mdw_drv_warn("not support\n");
		return -EINVAL;

#ifdef APUSYS_OPTIONS_DEVALLOC_SUPPORT
		if (copy_from_user(&dev_alloc, (void *)arg,
			sizeof(struct apusys_ioctl_dev))) {
			mdw_drv_err("copy dev_alloc struct fail\n");
			ret = -EINVAL;
			goto out;
		}

		/* get type device from resource mgr */
		memset(&acq, 0, sizeof(acq));
		acq.target_num = 1;
		acq.dev_type = dev_alloc.dev_type;
		acq.is_done = 0;
		acq.owner = user->open_pid;
		if (acq_device_sync(&acq) <= 0) {
			mdw_drv_err("alloc device(%d) by user(%p/0x%llx) fail\n",
				dev_alloc.dev_type,
				user, user->id);

			ret = -ENODEV;
			goto out;
		}

		/* insert allocated device to user list */
		count = 0;
		list_for_each_safe(list_ptr, tmp, &acq.dev_info_list) {
			dev_info = list_entry(list_ptr,
			struct apusys_dev_info, acq_list);

			ret = apusys_user_insert_dev(user,
				dev_info);
			if (ret) {
				mdw_drv_err("insert dev list fail\n");
				if (put_device_lock(dev_info)) {
					mdw_drv_err("put dev fail\n");
					ret = -ENODEV;
				}
				break;
			}
			count++;
		}

		dev_alloc.handle = (uint64_t)dev_info;
		dev_alloc.dev_idx = (uint64_t)dev_info->dev->idx;
		if (copy_to_user((void *)arg, &dev_alloc,
			sizeof(struct apusys_ioctl_dev))) {
			mdw_drv_err("copy user dev struct to user fail\n");
			ret = -EINVAL;
		}
		break;
#endif

	case APUSYS_IOCTL_DEVICE_FREE:
		mdw_lne_debug();
		mdw_drv_warn("not support\n");
		return -EINVAL;

#ifdef APUSYS_OPTIONS_DEVALLOC_SUPPORT
		if (copy_from_user(&dev_alloc, (void *)arg,
			sizeof(struct apusys_ioctl_dev))) {
			mdw_drv_err("copy dev_alloc struct fail\n");
			ret = -EINVAL;
			goto out;
		}

		/* check parameters */
		if (dev_alloc.num == 0 || dev_alloc.handle == 0 ||
			dev_alloc.dev_idx < 0) {
			mdw_drv_err("invalid parameter(%d/0x%llx/%d)\n",
				dev_alloc.num,
				dev_alloc.handle,
				dev_alloc.dev_idx);

			ret = -EINVAL;
			goto out;
		}

		/* put device back to dev table */
		dev_info = apusys_user_get_dev(user, dev_alloc.handle);
		if (dev_info == NULL) {
			mdw_drv_err("can't find device(%d/0x%llx) user(%p)\n",
				dev_alloc.dev_type, dev_alloc.handle, user);
			ret = -ENODEV;
			goto out;
		}
		ret = put_device_lock(dev_info);
		if (ret) {
			mdw_drv_err("free device(%p) fail(%d)\n",
				dev_info->dev, ret);
			ret = -ENODEV;
		} else {
			/* delete device from user */
			if (apusys_user_delete_dev(user, dev_info)) {
				mdw_drv_err("del dev(%p/0x%llx) ret(%d)\n",
					dev_info->dev,
					user->id,
					ret);

				ret = -ENODEV;
			}
		}
		break;
#endif

	case APUSYS_IOCTL_FW_LOAD:
		mdw_lne_debug();
		if (copy_from_user(&ioctl_fw,
			(void *)arg, sizeof(struct apusys_ioctl_fw))) {
			mdw_drv_err("copy fw struct fail\n");
			ret = -EINVAL;
			goto out;
		}

		memset(&kmem, 0, sizeof(struct apusys_kmem));

		/* ref count++ */
		kmem.fd = ioctl_fw.mem_fd;
		if (apusys_mem_map_kva(&kmem)) {
			mdw_drv_err("map cmd buffer kva from fd(%d)fail\n",
				ioctl_fw.mem_fd);
			ret = -EINVAL;
			goto out;
		}
		mdw_flw_debug("%x\n", ioctl_fw.magic);

		if (apusys_mem_map_iova(&kmem)) {
			mdw_drv_err("map cmd buffer iova from fd(%d)fail\n",
				ioctl_fw.mem_fd);
			ret = -EINVAL;
			goto out;
		}

		/* check size */
		if (ioctl_fw.size + ioctl_fw.offset > kmem.iova_size) {
			mdw_drv_err("check ucmd size(%u/%u/%u) fail\n",
				ioctl_fw.size,
				ioctl_fw.offset,
				kmem.iova_size);
			ret = -EINVAL;
			goto check_unfw_size_fail;
		}

		ret = res_load_firmware(ioctl_fw.dev_type,
			ioctl_fw.magic, ioctl_fw.name,
			ioctl_fw.idx, kmem.kva,
			kmem.iova, ioctl_fw.size,
			APUSYS_FIRMWARE_LOAD);
		if (ret) {
			mdw_drv_err("load fw to device(%d/%d) fail\n",
				ioctl_fw.dev_type,
				ioctl_fw.idx);
		}

check_unfw_size_fail:
		/* ref count-- */
		if (apusys_mem_unmap_iova(&kmem)) {
			mdw_drv_err("uni fw(%d/0x%llx/%s) fail\n",
			ioctl_fw.mem_fd, kmem.kva, ioctl_fw.name);
		}

		/* ref count-- */
		if (apusys_mem_unmap_kva(&kmem)) {
			mdw_drv_err("unmap fw(%d/0x%llx/%s) fail\n",
			ioctl_fw.mem_fd, kmem.kva, ioctl_fw.name);
		}
		break;

	case APUSYS_IOCTL_FW_UNLOAD:
		mdw_lne_debug();
		if (copy_from_user(&ioctl_fw, (void *)arg,
			sizeof(struct apusys_ioctl_fw))) {
			mdw_drv_err("copy fw struct fail\n");
			ret = -EINVAL;
			goto out;
		}
		memset(&kmem, 0, sizeof(struct apusys_kmem));

		mdw_flw_debug("fd = %d/%d\n",
				kmem.fd,
			ioctl_fw.mem_fd);

		kmem.fd = ioctl_fw.mem_fd;
		if (apusys_mem_map_kva(&kmem)) {
			mdw_drv_err("map fw buffer kva from fd(%d)fail\n",
				ioctl_fw.mem_fd);
			ret = -EINVAL;
			goto out;
		}

		if (apusys_mem_map_iova(&kmem)) {
			mdw_drv_err("map cmd buffer iovva from fd(%d)fail\n",
				ioctl_fw.mem_fd);
			ret = -EINVAL;
			goto out;
		}

		ret = res_load_firmware(ioctl_fw.dev_type,
			ioctl_fw.magic, ioctl_fw.name,
			ioctl_fw.idx, kmem.kva,
			kmem.iova, ioctl_fw.size,
			APUSYS_FIRMWARE_UNLOAD);

		if (ret) {
			mdw_drv_err("load fw to device(%d/%d) fail\n",
				ioctl_fw.dev_type,
				ioctl_fw.idx);
		}

		/* ref count-- */
		if (apusys_mem_unmap_iova(&kmem)) {
			mdw_drv_err("uni fw(%d/0x%llx/%s) fail\n",
			ioctl_fw.mem_fd, kmem.kva, ioctl_fw.name);
		}

		if (apusys_mem_unmap_kva(&kmem)) {
			mdw_drv_err("unmap fw buffer kva from fd(%d)fail\n",
				ioctl_fw.mem_fd);
			ret = -EINVAL;
		}

		break;

	case APUSYS_IOCTL_USER_CMD:
		mdw_lne_debug();
		if (copy_from_user(&ioctl_ucmd, (void *)arg,
			sizeof(struct apusys_ioctl_ucmd))) {
			mdw_drv_err("copy usercmd struct fail\n");
			return -EINVAL;
		}

		/* map kva */
		memset(&kmem, 0, sizeof(struct apusys_kmem));
		kmem.size = ioctl_ucmd.size;
		kmem.fd = ioctl_ucmd.mem_fd;
		if (apusys_mem_map_kva(&kmem)) {
			mdw_drv_err("map cmdbuf kva from fd(%d)fail\n",
				ioctl_ucmd.mem_fd);
			return -ENOMEM;
		}

		/* map iova */
		if (apusys_mem_map_iova(&kmem)) {
			mdw_drv_err("map cmdbuf iova from fd(%d)fail\n",
				ioctl_ucmd.mem_fd);
			if (apusys_mem_unmap_kva(&kmem)) {
				mdw_drv_err("unmap cmdbuf kva from fd(%d)fail\n",
					ioctl_ucmd.mem_fd);
			}
			return -ENOMEM;
		}

		/* check size */
		if (ioctl_ucmd.size + ioctl_ucmd.offset > kmem.iova_size) {
			mdw_drv_err("check ucmd size(%u/%u/%u) fail\n",
				ioctl_ucmd.size,
				ioctl_ucmd.offset,
				kmem.iova_size);
			ret = -EINVAL;
			goto check_ucmd_size_fail;
		}

		/* send ucmd to driver */
		ret = res_send_ucmd(ioctl_ucmd.dev_type, ioctl_ucmd.idx,
				kmem.kva, kmem.iova, kmem.size);

check_ucmd_size_fail:
		/* unmap iova */
		if (apusys_mem_unmap_iova(&kmem)) {
			mdw_drv_err("unmap cmdbuf iova from fd(%d)fail\n",
				ioctl_ucmd.mem_fd);
			if (apusys_mem_unmap_kva(&kmem)) {
				mdw_drv_err("unmap cmdbuf kva from fd(%d)fail\n",
					ioctl_ucmd.mem_fd);
			}
			return -ENOMEM;
		}

		/* unmap kva */
		if (apusys_mem_unmap_kva(&kmem)) {
			mdw_drv_err("unmap cmdbuf kva from fd(%d)fail\n",
				ioctl_ucmd.mem_fd);
			return -ENOMEM;
		}

		break;

	case APUSYS_IOCTL_SEC_DEVICE_LOCK:
		mdw_lne_debug();
		if (copy_from_user(&ioctl_sec, (void *)arg,
			sizeof(struct apusys_ioctl_sec))) {
			mdw_drv_err("[sec]copy sec struct fail\n");
			ret = -EINVAL;
			goto out;
		}

		/* check device supported */
		dev_num = res_get_device_num(ioctl_sec.dev_type);
		if (dev_num <= 0) {
			mdw_drv_err("[sec]lock: not support dev(%d)\n",
				ioctl_sec.dev_type);
			ret = -ENODEV;
			goto out;
		}

		mdw_drv_warn("[sec]lock dev(%d) %d cores\n",
			ioctl_sec.dev_type, dev_num);

		/* get type device from resource mgr */
		memset(&acq, 0, sizeof(acq));
		acq.target_num = dev_num;
		acq.dev_type = ioctl_sec.dev_type;
		acq.is_done = 0;
		acq.owner = APUSYS_DEV_OWNER_SECURE;
		if (acq_device_sync(&acq) <= 0) {
			mdw_drv_err("[sec]lock alloc dev(%d) u(0x%llx) fail\n",
				ioctl_sec.dev_type,
				user->id);

			ret = -ENODEV;
			goto out;
		}

		mdw_drv_debug("[sec]lock dev(%d) %d cores done\n",
			ioctl_sec.dev_type, dev_num);

		/*
		 * record device lock and call power on
		 * with timeout=0 and boost=100
		 */
		count = 0;
		list_for_each_safe(list_ptr, tmp, &acq.dev_info_list) {
			dev_info = list_entry(list_ptr,
				struct apusys_dev_info, acq_list);
			/* power off */
			ret = res_power_off(dev_info->dev->dev_type,
					dev_info->dev->idx);
			if (ret) {
				mdw_drv_err("[sec]dev(%s-#%d) poweroff fail\n",
					dev_info->name,
					dev_info->dev->idx);
				break;
			}

			/* power on */
			ret = res_power_on(dev_info->dev->dev_type,
					dev_info->dev->idx, 100,
					APUSYS_SETPOWER_TIMEOUT_ALLON);
			if (ret) {
				mdw_drv_err("[sec]dev(%s-#%d) poweron fail\n",
					dev_info->name,
					dev_info->dev->idx);
				break;
			}

			/* record sec device acquire */
			ret = apusys_user_insert_secdev(user, dev_info);
			if (ret) {
				mdw_drv_err("[sec]insert dev list fail\n");
				if (put_device_lock(dev_info)) {
					mdw_drv_err("put dev fail\n");
					ret = -ENODEV;
				}
				break;
			}

			/* TODO: call mtee enable 1 */
			count++;
		}

		if (count != dev_num) {
			mdw_drv_warn("[sec]alloc dev num confuse(%d/%d)\n",
				count, dev_num);
		}

		/* check success */
		if (ret == 0) {
			if (res_secure_on(ioctl_sec.dev_type)) {
				mdw_drv_err("[sec]dev(%d) secure mode on fail\n",
					ioctl_sec.dev_type);
				ret = -ENODEV;
			} else {
				mdw_drv_debug("[sec]power on dev(%d) %d cores ok\n",
					ioctl_sec.dev_type, dev_num);
				secure_perf_raise();
				break;
			}
		}

		/* if fail, need to release power and device */
		mdw_drv_warn("[sec]release dev(%d)...\n", ioctl_sec.dev_type);
		val = 0;
		list_for_each_safe(list_ptr, tmp, &acq.dev_info_list) {
			dev_info = list_entry(list_ptr,
				struct apusys_dev_info, acq_list);

			/* power off */
			if (res_power_off(dev_info->dev->dev_type,
					dev_info->dev->idx)) {
				mdw_drv_err("[sec]dev(%s-#%d) poweroff fail\n",
					dev_info->name,
					dev_info->dev->idx);
				ret = -ENODEV;
			}

			if (put_device_lock(dev_info)) {
				mdw_drv_err("[sec]put dev(%s-#%d) fail\n",
					dev_info->name,
					dev_info->dev->idx);
				ret = -ENODEV;
			}

			if (val < count) {
				if (apusys_user_delete_secdev(user, dev_info)) {
					mdw_drv_err("[sec]del secdev(%s-#%d)fail\n",
						dev_info->name,
						dev_info->dev->idx);
					ret = -ENODEV;
				}
			}
			val++;
		}

		mdw_drv_warn("[sec]release sec dev(%d) done(%d/%d)\n",
			ioctl_sec.dev_type,
			val, count);

		break;

	case APUSYS_IOCTL_SEC_DEVICE_UNLOCK:
		mdw_lne_debug();
		if (copy_from_user(&ioctl_sec, (void *)arg,
			sizeof(struct apusys_ioctl_sec))) {
			mdw_drv_err("copy sec struct fail\n");
			ret = -EINVAL;
			goto out;
		}

		/* check device supported */
		dev_num = res_get_device_num(ioctl_sec.dev_type);
		if (dev_num <= 0) {
			mdw_drv_err("sec unlock: not support dev(%d)\n",
				ioctl_sec.dev_type);
			ret = -ENODEV;
			goto out;
		}

		mdw_drv_warn("[sec]unlock dev(%d) %d cores\n",
			ioctl_sec.dev_type, dev_num);

		secure_perf_restore();

		/* TODO: call mtee enable 0 */
		ret = res_secure_off(ioctl_sec.dev_type);
		if (ret) {
			mdw_drv_err("dev(%d) secure mode off fail\n",
				ioctl_sec.dev_type);
		}

		/* power off all device by type */
		for (count = 0; count < dev_num; count++) {
			if (res_power_off(ioctl_sec.dev_type, count)) {
				mdw_drv_err("sec unlock poweroff dev(%d/%d) fail\n",
					ioctl_sec.dev_type, count);
				ret = -ENODEV;
			}
		}

		/* delete secure type from user */
		if (apusys_user_delete_sectype(user, ioctl_sec.dev_type)) {
			mdw_drv_err("sec unlock del stype(%d) from u list fail\n",
				ioctl_sec.dev_type);
			ret = -EINVAL;
		}

		mdw_drv_debug("[sec]unlock dev(%d) %d cores done\n",
			ioctl_sec.dev_type, dev_num);

		break;

	default:
		ret = -EINVAL;
		break;
	}

out:
	return ret;
}

static long apusys_compat_ioctl(struct file *flip, unsigned int cmd,
	unsigned long arg)
{
	switch (cmd) {
	case APUSYS_IOCTL_HANDSHAKE:
	case APUSYS_IOCTL_MEM_ALLOC:
	case APUSYS_IOCTL_MEM_FREE:
	case APUSYS_IOCTL_MEM_IMPORT:
	case APUSYS_IOCTL_MEM_UNIMPORT:
	case APUSYS_IOCTL_MEM_CTL:
	case APUSYS_IOCTL_RUN_CMD_SYNC:
	case APUSYS_IOCTL_RUN_CMD_ASYNC:
	case APUSYS_IOCTL_WAIT_CMD:
	case APUSYS_IOCTL_SET_POWER:
	case APUSYS_IOCTL_DEVICE_ALLOC:
	case APUSYS_IOCTL_DEVICE_FREE:
	case APUSYS_IOCTL_FW_LOAD:
	case APUSYS_IOCTL_FW_UNLOAD:
	case APUSYS_IOCTL_USER_CMD:
	case APUSYS_IOCTL_SEC_DEVICE_LOCK:
	case APUSYS_IOCTL_SEC_DEVICE_UNLOCK:
	{
		return flip->f_op->unlocked_ioctl(flip, cmd,
					(unsigned long)compat_ptr(arg));
	}
	default:
		return -ENOIOCTLCMD;
		/*return vpu_ioctl(flip, cmd, arg);*/
	}

	return 0;
}

#ifdef CONFIG_PM
int midware_pm_suspend(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);
	return apusys_suspend(pdev, PMSG_SUSPEND);
}

int midware_pm_resume(struct device *device)
{
	struct platform_device *pdev = to_platform_device(device);

	WARN_ON(pdev == NULL);
	return apusys_resume(pdev);
}

int midware_pm_restore_noirq(struct device *device)
{
	return 0;
}

static const struct dev_pm_ops apusys_pm_ops = {
	.suspend = midware_pm_suspend,
	.resume = midware_pm_resume,
	.freeze = midware_pm_suspend,
	.thaw = midware_pm_resume,
	.poweroff = midware_pm_suspend,
	.restore = midware_pm_resume,
	.restore_noirq = midware_pm_restore_noirq,
};
#endif
static struct platform_driver midware_driver = {
	.probe = apusys_probe,
	.remove = apusys_remove,
	.suspend = apusys_suspend,
	.resume  = apusys_resume,
	.driver = {
		.name = APUSYS_DEV_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &apusys_pm_ops,
#endif
	},
};

static void midware_device_release(struct device *dev)
{
}

static struct platform_device midware_device = {
	.name = APUSYS_DEV_NAME,
	.id = 0,
	.dev = {
		.release = midware_device_release,
		},
};

int midware_init(struct apusys_core_info *info)
{
#ifdef APUSYS_OPTIONS_INF_POWERARCH
	if (!apusys_power_check()) {
		mdw_drv_err("apusys disable\n");
		return -ENODEV;
	}
#endif

	if (info == NULL) {
		mdw_drv_err("invalid core info\n");
		return -EINVAL;
	}
	g_core_info = info;

	if (platform_driver_register(&midware_driver)) {
		mdw_drv_err("failed to register apusys midware driver");
		return -ENODEV;
	}

	if (platform_device_register(&midware_device)) {
		mdw_drv_err("failed to register apusys midware device");
		platform_driver_unregister(&midware_driver);
		return -ENODEV;
	}

	return 0;
}

void midware_exit(void)
{
	platform_driver_unregister(&midware_driver);
	platform_device_unregister(&midware_device);
}
