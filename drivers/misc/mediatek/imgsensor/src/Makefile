# SPDX-License-Identifier: GPL-2.0

subdir-ccflags-y += -I$(srctree)/drivers/i2c/busses/ \
             -I$(srctree)/drivers/misc/mediatek/ccu/inc

ifneq ($(subst ",,$(CONFIG_MTK_PLATFORM)),)
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
else ifneq ($(subst ",,$(CONFIG_MTK_DISP_PLATFORM)),)
MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_DISP_PLATFORM))
endif

ARCH_MTK_PROJECT := $(subst ",,$(CONFIG_ARCH_MTK_PROJECT))

MTK_PLATFORM_PATH_ROOT := $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)
MTK_PROJECT_PATH_ROOT := $(srctree)/drivers/misc/mediatek/mach/$(MTK_PLATFORM)/$(ARCH_MTK_PROJECT)

CONFIG_MTK_PLATFORM_SENSOR := $(CONFIG_MTK_PLATFORM)

subdir-ccflags-y += -I$(MTK_PROJECT_PATH_ROOT)/camera/camera

MTK_DRIVER_PATH  := $(srctree)/drivers/misc/mediatek

subdir-ccflags-y += -I$(MTK_DRIVER_PATH)/camera/$(MTK_PLATFORM) \
             -I$(MTK_DRIVER_PATH)/imgsensor/inc \
             -I$(MTK_DRIVER_PATH)/cam_cal/inc \
             -I$(MTK_DRIVER_PATH)/smi \
             -I$(MTK_DRIVER_PATH)/include/mt-plat/$(MTK_PLATFORM)/include/mach \
             -I$(MTK_DRIVER_PATH)/imgsensor/src/$(MTK_PLATFORM)


CUSTOM_KERNEL_FLAGS := $(addprefix -D, $(shell echo \
             $(CONFIG_CUSTOM_KERNEL_IMGSENSOR) | tr a-z A-Z))
subdir-ccflags-y += $(CUSTOM_KERNEL_FLAGS)

ifneq ($(MTK_PLATFORM),)
obj-y += $(MTK_PLATFORM)/
endif

