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

ifeq ($(CONFIG_MTK_GCOV_KERNEL),y)
GCOV_PROFILE := y
endif

MTK_PLATFORM := $(subst ",,$(CONFIG_MTK_PLATFORM))
ccflags-y += -Idrivers/misc/mediatek/mtprof
ifdef CONFIG_MTK_DRAM_LOG_STORE
subdir-ccflags-y += -Idrivers/misc/mediatek/log_store/
endif
subdir-ccflags-y += -I$(srctree)/drivers/misc/mediatek/include/mt-plat
LINUXINCLUDE += -include $(srctree)/kernel/sched/sched.h

obj-$(CONFIG_MTPROF) = bootprof.o
