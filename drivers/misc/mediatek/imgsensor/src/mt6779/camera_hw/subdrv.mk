# SPDX-License-Identifier: GPL-2.0

MKFILE_PATH := $(lastword $(MAKEFILE_LIST))
CURRENT_DIR := $(patsubst %/,%,$(dir $(realpath $(MKFILE_PATH))))
RELPATH := ..$(subst $(IMGSENSOR_DRIVER_PATH),,$(CURRENT_DIR))

imgsensor-objs += \
	$(RELPATH)/imgsensor_cfg_table.o \
	$(RELPATH)/regulator/regulator.o \
	$(RELPATH)/gpio/gpio.o \
	$(RELPATH)/mclk/mclk.o \

