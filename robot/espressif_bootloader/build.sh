#!/usr/bin/env bash
export PATH=/opt/xtensa-lx106-elf/bin:$PATH
make SDK_BASE=../espressif/ COMPILE=gcc
python3 bootloaderConfig.py 256 50
