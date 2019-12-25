# SPDX-License-Identifier: GPL-2.0

ifeq ($(CONFIG_MTK_GCOV_KERNEL),y)
GCOV_PROFILE_mtk_battery_percentage_throttling.o := y
endif

obj-$(CONFIG_MTK_BATTERY_OC_POWER_THROTTLING) += mtk_battery_oc_throttling.o
obj-$(CONFIG_MTK_BATTERY_PERCENTAGE_POWER_THROTTLING) += mtk_battery_percentage_throttling.o
obj-$(CONFIG_MTK_BATTERY_PERCENTAGE_POWER_THROTTLING) += mtk_battery_percentage_throttling_debug.o
obj-$(CONFIG_MTK_LOW_BATTERY_POWER_THROTTLING) += mtk_low_battery_throttling.o
obj-$(CONFIG_PMIC_LBAT_SERVICE) += pmic_lbat_service.o
