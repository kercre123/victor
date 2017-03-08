#!/usr/bin/env bash

echo ">>> Configuring Linux Gyp"
cd /vagrant
./configure.py --platform linux --projectRoot "/vagrant" --with-clad ../../tools/message-buffers

echo ">>> Running Valgrind on Linux"
./project/buildServer/steps/valgrindTest.py --platform linux --mode all --projectRoot "/vagrant"
