ifeq ($(CONFIG_MTK_GCOV_KERNEL),y)
GCOV_PROFILE := y
endif
ccflags-y += -I$(srctree)/drivers/misc/mediatek/sspm/

mtk-dvfsrc-help-objs = dvfsrc.o dvfsrc-opp.o dvfsrc-sysfs.o dvfsrc-ipi.o
mtk-dvfsrc-help-objs += dvfsrc-mt6779.o dvfsrc-mt6779-met.o

obj-$(CONFIG_MTK_DVFSRC) += mtk-dvfsrc-help.o
obj-$(CONFIG_MTK_DVFSRC) += dvfsrc-boost.o
