#!/bin/bash

set -e

# how are we going to do this...

VIC_PATH="$(pwd)/.."

# stage

cd "${VIC_PATH}"
./project/victor/scripts/stage.sh -c Release

cd ota/wire-os
./main mount -ota="2.0.1.0d"

OTA_PATH="$(pwd)/work"
STAGING_PATH="${VIC_PATH}/_build/staging/Release"

echo "Removing old anki folder..."
rm -rf "${OTA_PATH}/anki"

echo "Copying anki folder from staging..."
cp -rp "${STAGING_PATH}/anki" "${OTA_PATH}/"

./main patch -output="$1" -target=0

./main pack