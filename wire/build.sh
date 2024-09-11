#!/bin/bash

set -e

cd ~
if [[ ! -d .anki ]]; then
    echo "Downloading ~/.anki folder contents
    mkdir -p .anki
    cd .anki
    curl https://archive.org/anki-contents.tar.gz | tar -zx
fi
cd ~/victor

source project/victor/envsetup.sh
source project/victor/scripts/usefulALiases.sh

victor_build_debugo2
