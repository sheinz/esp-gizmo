PROGRAM = esp-gizmo
PROGRAM_SRC_DIR = ./src
EXTRA_COMPONENTS := extras/i2c extras/bmp280 extras/dht extras/bmp180
EXTRA_COMPONENTS += extras/rboot-ota extras/paho_mqtt_c

VARIANT ?= switch
DEVICE_IP ?= 192.168.0.108

ifeq ($(VARIANT), sensor)
	PROGRAM_SRC_DIR += variant/sensor
else ifeq ($(VARIANT), switch)
	PROGRAM_SRC_DIR += variant/switch
endif

include ../esp-open-rtos/common.mk

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)  # OSX
upload: all
	@echo "Uploading $(DEVICE_IP)..."
	@-echo "mode binary\nput firmware/$(PROGRAM).bin firmware.bin\nquit"\
		| tftp $(DEVICE_IP) | true # ignore tftp return code
	@echo "Upload finished"
endif
ifeq ($(UNAME), Linux)
	# TODO: check upload in linux 
endif
