#!/bin/bash

set -e

if [[ ! -f ./CPPLINT.cfg ]]; then
    if [[ -f ../CPPLINT.cfg ]]; then
        cd ..
    else
        echo "This script must be run in the victor repo. ./wire/build.sh"
        exit 1
    fi
fi

VICDIR="$(pwd)"

cd ~
if [[ ! -d .anki ]]; then
    echo "Downloading ~/.anki folder contents..."
    git clone https://github.com/kercre123/anki-deps
    mv anki-deps .anki
fi

cd "${VICDIR}"

git lfs update --force

echo "Building victor..."

./project/victor/scripts/victor_build_release.sh

echo "Copying vic-cloud and vic-gateway..."
cp bin/* _build/vicos/Release/bin/

echo "Copying pv_server binary..."
cp 3rd/picovoice/vicos/bin/pv_server _build/vicos/Release/bin/
chmod +rwx _build/vicos/Release/bin/pv_server

echo

echo "Build was successful!"
