#
# Copyright (C) 2015 MediaTek Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#

ccflags-y += -I$(srctree)
THERMAL_CHIP_DRIVER_DIR := $(srctree)/drivers/misc/mediatek/thermal/mt6779
ccflags-y  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/include/
ccflags-y  += -I$(srctree)/drivers/gpu/mediatek/gpufreq/include/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/gpu/ged/include

ifeq ($(CONFIG_MTK_LEGACY_THERMAL), m)
	obj-$(CONFIG_MTK_LEGACY_THERMAL) += mtk_thermal_monitor_mod.o
	obj-$(CONFIG_MTK_LEGACY_THERMAL) += mtk_cooler_shutdown_mod.o
	obj-$(CONFIG_MTK_LEGACY_THERMAL) += mtk_cooler_backlight_mod.o

	mtk_thermal_monitor_mod-objs += mtk_thermal_monitor.o
	mtk_cooler_shutdown_mod-objs += mtk_cooler_shutdown.o
	mtk_cooler_backlight_mod-objs += mtk_cooler_backlight.o
else
	obj-y += mtk_thermal_monitor.o
	obj-y += mtk_cooler_shutdown.o

	ifeq ($(CONFIG_MTK_LEDS), y)
    # setMaxbrightness() defined in mediatek/kernel/drivers/leds/leds_drv.c only built when CONFIG_MTK_LEDS is set
    	obj-y += mtk_cooler_backlight.o
	endif
	obj-y += mtk_cooler_kshutdown.o
endif


obj-$(CONFIG_MTK_LEGACY_THERMAL) += common/
obj-$(CONFIG_MTK_LEGACY_THERMAL) += $(subst ",,$(CONFIG_MTK_LEGACY_THERMAL_PLATFORM))/

