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

#include <linux/init.h>
#include <linux/io.h>

#include "apusys_core.h"
#include "sample_drv.h"
#include "sample_cmn.h"
#include "sample_inf.h"

/* define */
#define APUSYS_SAMPLE_DEV_NAME "apusys_sample"

static int sample_probe(struct platform_device *pdev)
{
	smp_drv_debug("+\n");
	sample_device_init();
	smp_drv_debug("-\n");

	return 0;
}

static int sample_remove(struct platform_device *pdev)
{
	smp_drv_debug("+\n");
	sample_device_destroy();
	smp_drv_debug("-\n");

	return 0;
}

static int sample_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int sample_resume(struct platform_device *pdev)
{
	return 0;
}


static struct platform_driver sample_driver = {
	.probe = sample_probe,
	.remove = sample_remove,
	.suspend = sample_suspend,
	.resume  = sample_resume,
	.driver = {
		.name = APUSYS_SAMPLE_DEV_NAME,
		.owner = THIS_MODULE,
	},
};

static void sample_device_release(struct device *dev)
{
}

static struct platform_device sample_device = {
	.name = APUSYS_SAMPLE_DEV_NAME,
	.id = 0,
	.dev = {
		.release = sample_device_release,
		},
};

int sample_init(struct apusys_core_info *info)
{
	if (platform_driver_register(&sample_driver)) {
		smp_drv_err("failed to register apusys sample driver)\n");
		return -ENODEV;
	}

	if (platform_device_register(&sample_device)) {
		smp_drv_err("failed to register apusys sample device\n");
		platform_driver_unregister(&sample_driver);
		return -ENODEV;
	}

	return 0;
}

void sample_exit(void)
{
	platform_driver_unregister(&sample_driver);
	platform_device_unregister(&sample_device);
}

