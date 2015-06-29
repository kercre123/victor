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

$PYTHON $ESPTOOL --port $1 --baud 230400 write_flash 0x00000 bin/eagle.flash.bin 0x40000 bin/eagle.irom0text.bin
