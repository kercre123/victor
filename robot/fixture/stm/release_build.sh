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

#clean
echo cleaning
rm -rf build
rm -f fixture.safe #XXX: DELETE ALL 'fixture*.safe' files

#clear debug flag for release build
echo clear debug flag
echo "#define NOT_FOR_FACTORY 0" > $flags_file
sleep 1 #delay for file system changes to clear cache

#keil command line opts: http://www.keil.com/support/man/docs/uv4/uv4_commandline.htm
echo building project
Tstart=$(($(date +%s%N)/1000000))
timeout 90.0s $keil -b $project -j0
errlvl=$?
Tend=$(($(date +%s%N)/1000000))
if [ $errlvl = 0 ]; then result="succeeded"; else result="FAILED"; fi
echo build: "("$errlvl")" $result in $(($Tend-$Tstart))ms

#restore debug flag
echo restore debug flag
echo "#define NOT_FOR_FACTORY 1" > $flags_file

#XXX: add release version to output safe filename

echo done
