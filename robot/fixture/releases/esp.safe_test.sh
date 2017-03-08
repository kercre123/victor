#!/usr/bin/env bash

#debug: uncomment to send console output to log file
exec 3>&1 4>&2
trap 'exec 2>&4 1>&3' 0 1 2 3
exec 1>log.txt 2>&1

#Make safe file from AXF outputs
python3 ../../tools/sign.py --prepend_size_word --rtip ./robot.axf --body ./syscon.axf ./factory.safe.test.fromaxf

#make safe file from BIN outputs
#must be generated directly from AXF! We are comparing final MD5 hash with this assumption
python3 ../../tools/sign.py --prepend_size_word --rtipbin ./robot.bin --bodybin ./syscon.bin ./factory.safe.test.frombin

md5sum factory.safe.test.fromaxf
md5sum factory.safe.test.frombin

#sleep 2
#read -p "Press a key to exit"
