ifeq ($(CONFIG_MTK_GCOV_KERNEL),y)
GCOV_PROFILE := y
endif

ccflags-y += -I$(srctree)/drivers/misc/mediatek/sspm/

obj-$(CONFIG_MTK_QOS_FRAMEWORK) += mtk_qos.o
mtk_qos-objs := qos-mt6779.o mtk_qos_common.o  mtk_qos_bound.o  mtk_qos_sysfs.o
