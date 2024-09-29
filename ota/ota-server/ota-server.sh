#!/bin/bash

cd /home/build/victor/ota/ota-server
PATH=$PATH:bin LD_LIBRARY_PATH=$LD_LIBRARY_PATH:lib64 ./main
