#!/usr/bin/env bash

export XTENSA_DIR=$PWD/../../tools/anki-util/tools/build-tools/tools/xtensa-lx106
if [[ `uname` == "Darwin" ]]; then
    #export PATH=$PWD/../../../esp-open-sdk/xtensa-lx106-elf/bin/:$PATH
    export PATH=$XTENSA_DIR/osx/xtensa-lx106-elf/bin/:$PATH
elif [[ `uname` == "Linux" ]]; then
    export PATH=$XTENSA_DIR/linux/xtensa-lx106-elf/bin/:$PATH
fi

cd app

clear

make COMPILE=gcc BOOT=new APP=1 SPI_SPEED=20 SPI_MODE=QIO SPI_SIZE_MAP=3 && \
make clean && \
make COMPILE=gcc BOOT=new APP=2 SPI_SPEED=20 SPI_MODE=QIO SPI_SIZE_MAP=3 && \
make clean
