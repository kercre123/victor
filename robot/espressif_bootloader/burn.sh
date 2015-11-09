#!/usr/bin/env bash

SDK_BASE=/Volumes/devel/GitHub/products-cozmo/robot/espressif

python2 $SDK_BASE/tools/esptool.py --port $1 --baud 115200 write_flash --flash_size 16m --flash_freq 80m \
        0x000000 firmware/rboot.bin \
        0x002000 firmware/bootloaderConfig.bin
