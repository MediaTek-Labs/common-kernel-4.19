#
# Copyright (C) 2019 MediaTek Inc.
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

MTK_PLATFORM := mt6779
PDIR = $(MTK_PLATFORM)

ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/1.0/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/apusys/power/1.0/$(PDIR)
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
ccflags-y += -I$(srctree)/drivers/misc/mediatek/base/power/include
ccflags-y += -I$(srctree)/drivers/misc/mediatek/smi/
ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat/$(PDIR)/include
ccflags-y += -I$(srctree)/drivers/devfreq
ccflags-y += -I$(srctree)/include

MTK_MDLA_CORE := 1
subdir-ccflags-y += -DMTK_MDLA_CORE=$(MTK_MDLA_CORE)
subdir-ccflags-y += -DMTK_MDLA_DVT

MTK_VPU_CORE := 1
ifneq (,$(filter $(strip $(MTK_PLATFORM)), mt6779))
MTK_VPU_CORE := 2
else
endif
subdir-ccflags-y += -DMTK_VPU_CORE=$(MTK_VPU_CORE)

obj-m += apu_power.o
apu_power-objs:= $(PDIR)/apu_dvfs.o \
		$(PDIR)/vpu_power_ctl.o $(PDIR)/mdla_dvfs.o \
		$(PDIR)/spm_mtcmos_ctl.o $(PDIR)/apu_platform_debug.o \
		apusys_power_driver.o apusys_power_debug.o


# PER_OBSERVER support
ifeq ($(CONFIG_MTK_PERF_OBSERVER),y)
subdir-ccflags-y += -DMTK_PERF_OBSERVER
endif

# HQA load
ifeq ($(CONFIG_BUILD_ARM64_DTB_OVERLAY_IMAGE_NAMES),"mediatek/evb6779_64_hqact")
subdir-ccflags-y += -DHQA_LOAD
endif

# HQA load
ifeq ($(CONFIG_BUILD_ARM64_DTB_OVERLAY_IMAGE_NAMES),"mediatek/k79v1_64_ctighten")
subdir-ccflags-y += -DHQA_LOAD
endif
