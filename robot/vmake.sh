#!/bin/bash
# Set the project folder/project file you want below (e.g., robot/robot.uvproj)
export PRJ=robot
cd "${0%/*}"
if [ ! -d ../$PRJ ]; then echo "Must run from project folder"; exit 1; fi

#command line options:
buildBoot=0
PROGRESS=""
for arg in "$@"; do
  if [ "${arg}" == "-b" ]; then buildBoot=1; fi #'-b' build bootloader
  if [ "${arg}" == "-p" ]; then PROGRESS="--progress"; fi #'-P' show rsync progress
  if [ "${arg}" == "clean" ]; then echo Cleaning...; rm -rf ../$PRJ/syscon/build; fi #'clean' = clean build
done

echo Copying to anki-vm-keil:$HOSTNAME/$PRJ...
rsync $PROGRESS --delete --exclude=fixture/ -rte "ssh" ../$PRJ nathan@anki-vm-keil:$HOSTNAME
rsync $PROGRESS --delete -rte "ssh" ../crypto nathan@anki-vm-keil:$HOSTNAME
echo Building...
ssh nathan@anki-vm-keil 'bash -lc "cd '"$HOSTNAME/$PRJ"';./convert_projx_to_uv4.sh";bash -lc "cd '"$HOSTNAME/$PRJ/syscon"';/cygdrive/c/Keil_v4/UV4/UV4.exe -j0 -b syscon.uvproj -o build.log;cat build.log;rm build.log"'
if [ $buildBoot -gt 0 ]; then
  echo Building Bootloader...
  ssh nathan@anki-vm-keil 'bash -lc "cd '"$HOSTNAME/$PRJ/syscon"';/cygdrive/c/Keil_v4/UV4/UV4.exe -j0 -b sysboot.uvproj -o build.log;cat build.log;rm build.log"'
fi
echo Copying back...
rsync $PROGRESS --exclude=fixture/ -rte "ssh" nathan@anki-vm-keil:$HOSTNAME/$PRJ ..
ls -la syscon/build/syscon_raw.bin
ls -la syscon/build/syscon.dfu
if [ $buildBoot -gt 0 ]; then ls -la syscon/build/sysboot.bin; fi
