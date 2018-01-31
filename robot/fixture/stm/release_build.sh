#!/bin/bash

keil=/mnt/c/Keil/UV4/UV4.exe
project=vicfix.uvproj
flags_file=app/app_build_flags.h
vers_file=app/app_release_ver.h

#require all instances of Keil (UV4.exe) to be closed before we allow build
tasklist=/mnt/c/Windows/System32/tasklist.exe
if $tasklist | grep -iq "UV4.exe"; then
  echo close all instances of UV4 before build
  exit 1
fi

#read release version
version=$(grep -oP '#define\s+APP_RELEASE_VERSION\s+\K([0-9]+)' $vers_file)
vlen=$(echo -n $version | wc -m)
echo release version \'$version\'

#clean
safefile=fixture.safe
manifest=$(printf fixture%03d $version).manifest
safefileVxx=$(printf fixture%03d $version).safe
zipfileVxx=$(printf firmware%03d $version).zip
echo cleaning
rm -rf build
rm -f $safefile $safefileVxx $zipfileVxx $manifest

#clear debug flag for release build
echo clear debug flag
echo "#define NOT_FOR_FACTORY 0" > $flags_file
sleep 1 #delay for file system changes to clear cache

#keil command line opts: http://www.keil.com/support/man/docs/uv4/uv4_commandline.htm
echo building project
Tstart=$(($(date +%s%N)/1000000))
timeout 90.0s $keil -b $project -j0
builderr=$?
Tend=$(($(date +%s%N)/1000000))

#restore debug flag
echo restore debug flag
echo "#define NOT_FOR_FACTORY 1" > $flags_file

#build the manifest
echo $(printf timestamp:%s "$(date +'%m/%d/%Y %H:%M:%S')") > $manifest
echo $(printf version:%d $version) >> $manifest
echo $(printf "build-time-ms:%d" $(($Tend-$Tstart))) >> $manifest
echo $(printf build-err:%d $builderr) >> $manifest

#package for shipment
if [ $builderr = 0 ]; then 
  echo build: "("$builderr")" succeess in $(($Tend-$Tstart))ms
  cp $safefile $safefileVxx
  #XXX: bootloader tools need update to support 'fixture###.safe' filenames
  #zip -9T $zipfileVxx $safefileVxx $manifest
  zip -9T $zipfileVxx $safefile $manifest
else
  echo build: "("$builderr")" ---FAILED--- in $(($Tend-$Tstart))ms
fi

echo done
