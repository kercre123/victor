#!/usr/bin/env bash

ESPTOOL=../../../coretech-external/espressif/esptool/esptool.py



python $ESPTOOL --port $1 --baud 230400 write_flash 0x00000 bin/eagle.flash.bin 0x40000 bin/eagle.irom0text.bin
