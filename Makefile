PROGRAM=esp-gizmo
OTA=1
DEVICE_IP=192.168.0.109
EXTRA_COMPONENTS=extras/rboot-ota
PROGRAM_SRC_DIR=./src
include ../esp-open-rtos/common.mk

upload:
	tftp -v -m octet $(DEVICE_IP) -c put firmware/$(PROGRAM).bin firmware.bin

console:
	picocom -b 115200 /dev/ttyUSB0
