#!/usr/bin/env bash

ESPTOOL=../../../coretech-external/espressif/esptool/esptool.py

python $ESPTOOL --port $1 --baud 230400 write_flash 0x00000 bin/boot_v1.2.bin \
                                                    0x01000 bin/upgrade/user1.512.new.0.bin \
                                                    0x41000 bin/upgrade/user2.512.new.0.bin \
                                                    0x7c000 bin/esp_init_data_default.bin \
                                                    0x7e000 bin/blank.bin
