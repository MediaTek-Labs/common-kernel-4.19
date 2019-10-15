#
# Copyright (C) 2018 MediaTek Inc.
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
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/gpu/hal/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/thermal/fakeHeader/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/sspm/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/sspm/mt6779
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/pmic/include/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/thermal/common/inc/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/include/
ccflags-y  += -I$(srctree)/include/dt-bindings/
ccflags-y  += -I$(srctree)/drivers/gpu/mediatek/gpufreq/include/



ccflags-y  += -I$(srctree)/drivers/misc/mediatek/video/include
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/gpu/ged/include
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/thermal/common/inc/
ccflags-y  += -I$(srctree)/drivers/power/mediatek/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/dfrc/
ifeq ($(CONFIG_MTK_TINYSYS_SSPM_SUPPORT),y)
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/sspm/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/sspm/mt6779
endif
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/vpu/mt6779
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/mdla/1.0
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/include
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y  += -I$(srctree)/drivers/gpu/mediatek/gpufreq/include/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/ppm_v3/src/mach/mt6779

ccflags-y  += -I$(srctree)/drivers/misc/mediatek/power/mt6779
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/dramc/mt6779
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/auxadc/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/
ccflags-y  += -I$(srctree)/drivers/misc/mediatek/imgsensor/inc/

ifeq ($(CONFIG_MTK_LEGACY_THERMAL), m)
	obj-$(CONFIG_MTK_LEGACY_THERMAL) += mtk_atm_mod.o
	mtk_atm_mod-objs += mtk_ts_cpu_noBank.o mtk_cooler_atm.o mtk_ta.o mtk_tc.o mtk_tc_wrapper.o ap_thermal_limit.o
else
	obj-y += mtk_tc.o
	obj-y += mtk_tc_wrapper.o
	obj-y += ap_thermal_limit.o
	obj-y += mtk_ta.o
	obj-y += mtk_cooler_atm.o
	obj-y += mtk_ts_cpu_noBank.o
endif
