#!/usr/bin/env bash

export PATH=/opt/xtensa-lx106-elf/bin:$PATH
export ESPDIR=`pwd`

# Clear the terminal so debugging builds is easier
clear

# Build the Espressif app
cd app
make COMPILE=gcc SPI_SPEED=80 SPI_MODE=QIO
