#!/usr/bin/env bash

export PATH=/opt/xtensa-lx106-elf/bin:$PATH
export ESPDIR=`pwd`

# Clear the terminal so debugging builds is easier
clear

# Echo
set -e

../tools/versionGenerator/versionGenerator.sh app/include/version.h

# Generate clad source
make esp -C ../clad -j4

# Run the logging pre-processor
../tools/ankiLogPP.py --preprocessor --string-table --output ../AnkiLogStringTables.json ../supervisor/src app

# Build the Espressif app
cd app
make COMPILE=gcc BOOT=new APP=1 SPI_SPEED=80 SPI_MODE=QIO SPI_SIZE_MAP=3
