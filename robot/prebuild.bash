#!/usr/bin/env bash
export PATH=/usr/bin:$PATH
# Build clad messages
make k02 -C clad -j4
# Run the logging pre-processor
tools/ankiLogPP.py --preprocessor --string-table --output AnkiLogStringTables.json supervisor/src k02_hal espressif/app sim_hal
# Generate the version file
bash tools/versionGenerator/versionGenerator.sh include/anki/cozmo/robot/version.h
