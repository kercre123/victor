#!/usr/bin/env bash
set -x
python3 sn.py $2
esptool.py --port $1 --baud 230400 write_flash --flash_size 16m --flash_freq 80m \
        0x000000 firmware/cboot.bin \
        0x001000 firmware/sn.bin \
        0x080000 ../staging/esp.factory.bin \
        0x0c8000 ../staging/factory.safe \
        0x1fc000 ../espressif/bin/esp_init_data_default.bin \
        0x1fe000 ../espressif/bin/blank.bin \
        0x003000 ../espressif/bin/blank.bin \
        0x103000 ../espressif/bin/blank.bin \
