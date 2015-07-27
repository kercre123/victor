#!/usr/bin/env bash

if [[ -d $CORETECH_EXTERNAL_DIR ]]; then
  ESPTOOL=$CORETECH_EXTERNAL_DIR/espressif/esptool/esptool.py
else
  echo "You must have CORETECH_EXTERNAL_DIR defined" 1>&2
  exit 1
fi

python2 $ESPTOOL --port $1 --baud 230400 write_flash 0x00000 bin/boot_v1.2.bin \
                                                     0x01000 bin/upgrade/user1.1024.new.2.bin \
                                                     0x81000 bin/upgrade/user2.1024.new.2.bin \
                                                     0xfc000 bin/esp_init_data_default.bin \
                                                     0xfe000 bin/blank.bin
