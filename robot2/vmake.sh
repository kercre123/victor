#!/bin/bash
# Set the project folder/project file you want below (e.g., robot/robot.uvproj)
export PRJ=robot2
cd "${0%/*}"
if [ ! -d ../$PRJ ]; then echo "Must run from project folder"; exit 1; fi
./syscon/tools/make_clad.sh

echo Copying to anki-vm-keil:$HOSTNAME/$PRJ...
rsync --delete -rte "ssh" ../$PRJ nathan@anki-vm-keil:$HOSTNAME
rsync --delete -rte "ssh" ../crypto nathan@anki-vm-keil:$HOSTNAME
echo Building...
ssh nathan@anki-vm-keil 'bash -lc "cd '"$HOSTNAME/$PRJ"';./convert_projx_to_uv4.sh";bash -lc "cd '"$HOSTNAME/$PRJ/syscon"';/cygdrive/c/Keil_v4/UV4/UV4.exe -j0 -b sysboot.uvproj -o build.log;cat build.log;rm build.log";bash -lc "cd '"$HOSTNAME/$PRJ/syscon"';/cygdrive/c/Keil_v4/UV4/UV4.exe -j0 -b syscon.uvproj -o build.log;cat build.log;rm build.log"'
echo Copying back...
rsync -rte "ssh" nathan@anki-vm-keil:$HOSTNAME/$PRJ ..
ls -la syscon/build/syscon.bin
