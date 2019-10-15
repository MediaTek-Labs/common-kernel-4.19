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
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/power/mt6779
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/dramc/mt6779
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/gpu/hal/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/auxadc/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/thermal/common/inc/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/include/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/imgsensor/inc/
ccflags-y  += -I$(srctree)/drivers/gpu/mediatek/gpufreq/include/

ifeq ($(CONFIG_MTK_LEGACY_THERMAL), m)
	ifneq (,$(filter $(CONFIG_MTK_LEGACY_THERMAL_PLATFORM), "mt6779"))
		obj-$(CONFIG_MTK_LEGACY_THERMAL) += mtk_ts_bts_mod.o
		obj-$(CONFIG_MTK_LEGACY_THERMAL) += mtk_ts_btsmdpa_mod.o
		obj-$(CONFIG_MTK_LEGACY_THERMAL) += mtk_monitor_tz_mod.o
		mtk_ts_bts_mod-objs += mtk_ts_bts.o
		mtk_ts_btsmdpa_mod-objs += mtk_ts_btsmdpa.o
		mtk_monitor_tz_mod-objs += mtk_monitor_tz.o
	endif
else
	ifneq (,$(filter $(CONFIG_MTK_LEGACY_THERMAL_PLATFORM), "mt6779"))
		obj-y += mtk_ts_bts.o
		obj-y += mtk_ts_btsmdpa.o
		obj-y += mtk_monitor_tz.o
	endif
endif