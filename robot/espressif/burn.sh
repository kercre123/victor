#!/usr/bin/env bash

if [[ -d $CORETECH_EXTERNAL_DIR ]]; then
  ESPTOOL=$CORETECH_EXTERNAL_DIR/espressif/esptool/esptool.py
else
  echo "You must have CORETECH_EXTERNAL_DIR defined" 1>&2
  exit 1
fi

if which python3; then
  PYTHON=python2
else
  PYTHON=python
fi

$PYTHON $ESPTOOL --port $1 --baud 230400 write_flash 0x00000 bin/boot_v1.2.bin \
                                                     0x01000 bin/upgrade/user1.1024.new.0.bin \
                                                     0x41000 bin/upgrade/user2.1024.new.0.bin \
                                                     0x7c000 bin/esp_init_data_default.bin \
                                                     0x7e000 bin/blank.bin
