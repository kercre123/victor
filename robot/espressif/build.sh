#!/usr/bin/env bash

cd app
clear
make COMPILE=gcc BOOT=new APP=1 SPI_SPEED=20 SPI_MODE=QIO SPI_SIZE_MAP=3 && \
make clean && \
make COMPILE=gcc BOOT=new APP=2 SPI_SPEED=20 SPI_MODE=QIO SPI_SIZE_MAP=3 && \
make clean
