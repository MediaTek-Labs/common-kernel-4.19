PLAT_TOP = $(srctree)/drivers/misc/mediatek/include/mt-plat

ccflags-y += -I$(src)/src/ \
	     -I$(srctree)/include/ \
	     -I$(PLAT_TOP)/

ifneq ( , $(filter y m, $(CONFIG_MTK_SRCLKEN_RC)))
    ccflags-y += -I$(src)/src/ \
	     -I$(srctree)/include/ \
	     -I$(CLKBUF_TOP)/ \
	     -I$(PLAT_TOP)/

    obj-$(CONFIG_MTK_SRCLKEN_RC) += srclken.o

    srclken-$(CONFIG_MTK_SRCLKEN_RC) += src/mtk-srclken-rc.o \
			src/mtk-srclken-rc-hw.o \
			mtk-srclken-bridge.o
else
    ccflags-y += -I$(srctree)/include/ \
	     -I$(PLAT_TOP)/

    obj-y += mtk-srclken-bridge.o
endif

