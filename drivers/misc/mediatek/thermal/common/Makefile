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

THERMAL_CHIP_DRIVER_DIR := $(srctree)/drivers/misc/mediatek/thermal/mt6779
ccflags-y  += -I$(THERMAL_CHIP_DRIVER_DIR)/inc
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/include/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/gpu/hal/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/thermal/common/inc/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/vpu/mt6779
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/mdla/1.0
ccflags-y  += -I$(srctree)/drivers/gpu/mediatek/gpufreq/include/

ifeq ($(CONFIG_MTK_LEGACY_THERMAL), m)
	obj-$(CONFIG_MTK_LEGACY_THERMAL) += mtk_thermal_platform_mod.o
	obj-$(CONFIG_MTK_LEGACY_THERMAL) += mtk_thermal_timer_mod.o
	mtk_thermal_platform_mod-objs += mtk_thermal_platform.o
	mtk_thermal_timer_mod-objs += mtk_thermal_timer.o
else
	obj-y += mtk_thermal_platform.o
	obj-y += mtk_thermal_timer.o
endif

obj-$(CONFIG_MTK_LEGACY_THERMAL) += thermal_zones/
obj-$(CONFIG_MTK_LEGACY_THERMAL) += coolers/