# Linux driver folder
ccflags-y += -I$(srctree)/drivers/input/touchscreen/GT1151/$(CONFIG_GT1151_CONFIG)/


ifeq (m,$(CONFIG_TOUCHSCREEN_MTK_GT1151))
obj-m :=gt1x.o
gt1x-objs := gt1x_generic.o gt1x_tools.o gt1x_tpd.o gt1x_update.o tpd_control.o \
	    tpd_button.o tpd_calibrate.o tpd_default.o tpd_misc.o \
	    tpd_setting.o tpd_debug.o
else
obj-y := gt1x_generic.o gt1x_tools.o gt1x_tpd.o gt1x_update.o tpd_control.o \
	    tpd_button.o tpd_calibrate.o tpd_default.o tpd_misc.o \
	    tpd_setting.o tpd_debug.o
endif
