#!/usr/bin/env bash

#regenerate the esp.safe file from binaries in this directory
python3 ../../tools/sign.py --prepend_size_word --rtipbin ./robot.bin --bodybin ./syscon.bin ./esp.safe.bin
