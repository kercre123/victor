#!/bin/bash

set -e

UNAME=$(uname)

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

echo "Updating anki-deps..."
cd ~/.anki
git pull

if [[ -d ~/.anki/cmake/3.9.6 ]]; then
    echo "Removing old version of cmake"
    rm -rf ~/.anki/cmake
fi

if [[ ${UNAME} == "Darwin" ]]; then
    echo "Checking out macOS branch..."
    cd ~/.anki
    if [[ $(uname -a) == *"arm64"* ]]; then
        git checkout macos-arm
    else
        git checkout macos
    fi
    git lfs install
    git lfs pull
else
    if [[ $(uname -a) == *"aarch64"* ]]; then
       cd ~/.anki
       git checkout arm64-linux
    fi
fi

cd "${VICDIR}"

git lfs update --force

echo "Building victor..."

./project/victor/scripts/victor_build_release.sh

echo "Copying vic-cloud and vic-gateway..."
cp -a bin/* _build/vicos/Release/bin/
echo "Copying libopus..."
cp -a 3rd/opus/vicos/lib/libopus.so.0.7.0 _build/vicos/Release/lib/libopus.so.0
cp -a patch-libs/* _build/vicos/Release/lib/
echo "Copying sb_server binary..."
cp -a 3rd/snowboy/vicos/bin/sb_server _build/vicos/Release/bin/
chmod +rwx _build/vicos/Release/bin/sb_server

echo

echo "Build was successful!"
