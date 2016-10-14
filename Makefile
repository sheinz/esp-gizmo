PROGRAM = esp-gizmo
PROGRAM_SRC_DIR = ./src
EXTRA_COMPONENTS = extras/i2c extras/bmp280 extras/dht extras/bmp180

VARIANT ?= sensor

ifeq ($(VARIANT), sensor)
	PROGRAM_SRC_DIR += variant/sensor
else ifeq ($(VARIANT), switch)
	PROGRAM_SRC_DIR += variant/switch
endif

include ../esp-open-rtos/common.mk

