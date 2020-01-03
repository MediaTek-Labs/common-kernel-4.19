obj-$(CONFIG_RPMB) += rpmb.o
rpmb-objs += core.o
ifneq ($(findstring $(CONFIG_TRUSTONIC_TEE_SUPPORT), y m),)
ccflags-y += -I$(srctree)/drivers/char/rpmb/drrpmb_gp/public
ccflags-y += -I$(srctree)/drivers/tee/gud/410/MobiCoreDriver/public/
ccflags-y += -I$(srctree)/drivers/mmc/core
ccflags-y += -I$(srctree)/drivers/mmc/card
obj-$(CONFIG_RPMB) += rpmb-mtk.o
endif

ccflags-y += -D__CHECK_ENDIAN__
