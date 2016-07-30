#!/bin/bash
# Set the project folder/project file you want below (e.g., robot/robot.uvproj)
export PRJ=robot
cd "${0%/*}"
if [ ! -d ../$PRJ ]; then echo "Must run from project folder"; exit 1; fi
make dev || exit 1
echo Copying to anki-vm-keil:$HOSTNAME/$PRJ...
rsync --delete -rte "ssh" ../$PRJ nathan@anki-vm-keil:$HOSTNAME
echo Building...
ssh nathan@anki-vm-keil 'bash -lc "cd '"$HOSTNAME/$PRJ"';/cygdrive/c/Keil_v4/UV4/UV4.exe -j0 -b robot41.uvproj -o build.log;cat build.log;rm build.log"'
echo Copying back...
rsync -rte "ssh" nathan@anki-vm-keil:$HOSTNAME/$PRJ ..
make k02_fix
ls -la build/$PRJ.bin
