#!/bin/bash
# Set the project folder/project file you want below (e.g., robot/robot.uvproj)
export PRJ=robot
cd "${0%/*}"
if [ ! -d ../$PRJ ]; then echo "Must run from project folder"; exit 1; fi

#command line options:
buildBoot=0; clean=0; buildCube=0; PROGRESS=""
for arg in "$@"; do
  if [ "${arg}" == "-b" ]; then buildBoot=1; fi #'-b' build bootloader
  if [ "${arg}" == "-p" ]; then PROGRESS="--progress"; fi #'-P' show rsync progress
  if [ "${arg}" == "clean" ]; then clean=1; fi
  if [ "${arg}" == "cube"  ]; then buildCube=1; fi
done

#clean
if [ $clean -gt 0 ] && [ $buildCube -gt 0 ]; then echo Cleaning cube...; rm -rf ../$PRJ/cube_firmware/build; fi
if [ $clean -gt 0 ] && [ $buildCube -le 0 ]; then echo Cleaning syscon...; rm -rf ../$PRJ/syscon/build; fi

#sync to VM
echo Copying to anki-vm-keil:$HOSTNAME/$PRJ...
rsync $PROGRESS --delete --exclude=fixture/ -rte "ssh" ../$PRJ nathan@anki-vm-keil:$HOSTNAME
rsync $PROGRESS --delete -rte "ssh" ../crypto nathan@anki-vm-keil:$HOSTNAME

#build
if [ $buildCube -gt 0 ]; then #Cube
  echo Building Cube Application...
  ssh nathan@anki-vm-keil 'bash -lc "cd '"$HOSTNAME/$PRJ"';python ./tools/convert_keil_uv5_to_uv4.py cube_firmware/cube_application.uvprojx cube_firmware/cube_application.uvproj"'
  ssh nathan@anki-vm-keil 'bash -lc "cd '"$HOSTNAME/$PRJ/cube_firmware"';/cygdrive/c/Keil_v4/UV4/UV4.exe -j0 -b cube_application.uvproj -o build.log;cat build.log;rm build.log"'
  if [ $buildBoot -gt 0 ]; then
    echo Building Cube Bootloader...
    ssh nathan@anki-vm-keil 'bash -lc "cd '"$HOSTNAME/$PRJ"';python ./tools/convert_keil_uv5_to_uv4.py cube_firmware/cube_firmware.uvprojx cube_firmware/cube_firmware.uvproj"'
    ssh nathan@anki-vm-keil 'bash -lc "cd '"$HOSTNAME/$PRJ/cube_firmware"';/cygdrive/c/Keil_v4/UV4/UV4.exe -j0 -b cube_firmware.uvproj -o build.log;cat build.log;rm build.log"'
  fi
else #Syscon
  echo Building Syscon...
  ssh nathan@anki-vm-keil 'bash -lc "cd '"$HOSTNAME/$PRJ"';python ./tools/convert_keil_uv5_to_uv4.py syscon/syscon.uvprojx syscon/syscon.uvproj"'
  ssh nathan@anki-vm-keil 'bash -lc "cd '"$HOSTNAME/$PRJ/syscon"';/cygdrive/c/Keil_v4/UV4/UV4.exe -j0 -b syscon.uvproj -o build.log;cat build.log;rm build.log"'
  if [ $buildBoot -gt 0 ]; then
    echo Building Syscon Bootloader...
    ssh nathan@anki-vm-keil 'bash -lc "cd '"$HOSTNAME/$PRJ"';python ./tools/convert_keil_uv5_to_uv4.py syscon/sysboot.uvprojx syscon/sysboot.uvproj"'
    ssh nathan@anki-vm-keil 'bash -lc "cd '"$HOSTNAME/$PRJ/syscon"';/cygdrive/c/Keil_v4/UV4/UV4.exe -j0 -b sysboot.uvproj -o build.log;cat build.log;rm build.log"'
  fi
fi

#sync from VM
echo Copying back...
rsync $PROGRESS --exclude=fixture/ -rte "ssh" nathan@anki-vm-keil:$HOSTNAME/$PRJ ..

#file check
if [ $buildCube -gt 0 ]; then #Cube
  ls -la cube_firmware/build/app/cube_application.bin
  ls -la cube_firmware/build/cube.dfu
  if [ $buildBoot -gt 0 ]; then ls -la cube_firmware/build/cubeboot.bin; fi
else #Syscon
  ls -la syscon/build/syscon_raw.bin
  ls -la syscon/build/syscon.dfu
  if [ $buildBoot -gt 0 ]; then ls -la syscon/build/sysboot.bin; fi
fi
