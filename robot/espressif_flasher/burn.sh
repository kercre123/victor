#!/usr/bin/env bash
set -x
esptool.py --port $1 --baud 230400 load_ram firmware/flasher.bin
