#!/bin/bash
# Set the project folder/project file you want below (e.g., cube/cube.uvproj)
export PRJ=cube
cd "${0%/*}"
if [ ! -d ../$PRJ ]; then echo "Must run from project folder"; exit 1; fi
echo Copying to $HOSTNAME/$PRJ...
rsync --delete -rte "ssh" ../$PRJ nathan@anki-vm-keil:$HOSTNAME
echo Building...
ssh nathan@anki-vm-keil '/cygdrive/c/Keil_v5/UV4/UV4.exe -j0 -b '"$HOSTNAME/$PRJ/block.uvproj"' -o build.log ; cat '"$HOSTNAME/$PRJ/build.log"' ; rm '"$HOSTNAME/$PRJ/build.log"
echo Copying back...
rsync -rte "ssh" nathan@anki-vm-keil:$HOSTNAME/$PRJ ..
ls -la build/$PRJ.bin
