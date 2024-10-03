#!/bin/bash

set -e

# how are we going to do this...

if [[ $1 == "" ]]; then
    echo "provide a version pweaaaaseeee :3"
    exit 1
fi

VIC_PATH="$(pwd)/.."
OUT_PATH="$(pwd)/out/$1"
mkdir -p "${OUT_PATH}"
mkdir "${OUT_PATH}/dev"
mkdir "${OUT_PATH}/oskr"
mkdir "${OUT_PATH}/dvt3"

# stage

echo "Staging /anki files..."

cd "${VIC_PATH}"
./project/victor/scripts/stage.sh -c Release

cd ota/wire-os

if [[ ! -f store/2.0.1.0.ota ]]; then
    sudo ./main download -url="https://ota.pvic.xyz/base/2.0.1.0.ota"
fi

OTA_PATH="$(pwd)/work"
STAGING_PATH="${VIC_PATH}/_build/staging/Release"

for target in 0 2 4; do
    case $target in
        0)
            output_file="${OUT_PATH}/dev/$1.ota"
            ;;
        1)
            output_file="${OUT_PATH}/whiskey/$1.ota"
            ;;
        2)
            output_file="${OUT_PATH}/oskr/$1.ota"
            ;;
        4)
            output_file="${OUT_PATH}/dvt3/$1.ota"
            ;;
    esac

    sudo ./main mount -ota="2.0.1.0"
    echo "Removing old anki folder..."
    sudo rm -rf "${OTA_PATH}/anki"
    echo "Copying anki folder from staging..."
    sudo rsync -arp "${STAGING_PATH}/anki" "${OTA_PATH}/"
    sudo ./main patch -output="$1" -target=$target
    sudo ./main pack
    sudo chmod +rwx out/$1.ota
    mv out/$1.ota "$output_file"
done
