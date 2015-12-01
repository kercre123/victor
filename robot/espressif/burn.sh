#!/usr/bin/env bash

python2 tools/esptool.py --port $1 --baud 230400 write_flash --flash_size 16m --flash_freq 80m \
        0x003000 bin/upgrade/user1.2048.new.3.bin
