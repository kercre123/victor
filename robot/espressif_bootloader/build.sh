#!/usr/bin/env bash
export PATH=/opt/xtensa-lx106-elf/bin:$PATH
if test ! -d build; then mkdir build; fi
../tools/versionGenerator/versionGenerator.sh build/version.h
make SDK_BASE=../espressif/ COMPILE=gcc
python3 bootloaderConfig.py
