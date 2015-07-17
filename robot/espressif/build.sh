#!/usr/bin/env bash

export ESPDIR=`pwd`
cd app
clear
make COMPILE=gcc BOOT=new APP=1 SPI_SPEED=40 SPI_MODE=QIO SPI_SIZE_MAP=0 && \
make clean && \
make COMPILE=gcc BOOT=new APP=2 SPI_SPEED=40 SPI_MODE=QIO SPI_SIZE_MAP=0 && \
make clean
