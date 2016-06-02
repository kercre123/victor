#!/usr/bin/env bash
export PATH=/opt/xtensa-lx106-elf/bin:$PATH
if test ! -d build; then mkdir build; fi
make SDK_BASE=../espressif/ COMPILE=gcc
