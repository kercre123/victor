#!/usr/bin/env bash

export PATH=/opt/xtensa-lx106-elf/bin:$PATH
export ESPDIR=`pwd`

# Clear the terminal so debugging builds is easier
clear

# Generate clad source 
cd ../clad
make
cd -

# Build the Espressif app
cd app
make COMPILE=gcc BOOT=new APP=1 SPI_SPEED=80 SPI_MODE=QIO SPI_SIZE_MAP=3 && \
make clean && \
make COMPILE=gcc BOOT=new APP=2 SPI_SPEED=80 SPI_MODE=QIO SPI_SIZE_MAP=3 && \
make clean
