#!/usr/bin/env bash

if [[ -d $CORETECH_EXTERNAL_DIR ]]; then
  ESPTOOL=$CORETECH_EXTERNAL_DIR/espressif/esptool/esptool.py
else
  echo "You must have CORETECH_EXTERNAL_DIR defined" 1>&2
  exit 1
fi

python2 $ESPTOOL --port $1 --baud 230400 write_flash --flash_size 16m --flash_freq 20m \
        0x1fc000 bin/esp_init_data_default.bin \
        0x1fe000 bin/blank.bin \
        0x000000 bin/boot_v1.2.bin \
        0x001000 bin/upgrade/user1.2048.new.3.bin \
        0x081000 bin/upgrade/user2.2048.new.3.bin
