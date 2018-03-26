#!/bin/bash
cd "$(dirname ${BASH_SOURCE[0]})"

#files, config
keil=/mnt/c/Keil/UV4/UV4.exe
project=vicfix.uvproj
flags_file=app/app_build_flags.h
vers_file=app/app_release_ver.h
errcodes=error_codes.csv
safefile=fixture.safe
manifest=manifest
buildlog=build.log

#require all instances of Keil (UV4.exe) to be closed before we allow build
tasklist=/mnt/c/Windows/System32/tasklist.exe
if $tasklist | grep -iq "UV4.exe"; then
  echo close all instances of UV4 before build
  exit 1
fi

#git (relative) list of modified files
#uncomittedfilez=$(git diff-files --relative --name-only --ignore-space-at-eol)
uncomittedfilez=$(git ls-files -m)

#read release version
version=$(grep -oP '#define\s+APP_RELEASE_VERSION\s+\K([0-9]+)' $vers_file)
vlen=$(echo -n $version | wc -m)
echo release version \'$version\'

#output files
#safefileVxx=$(printf fixture%03d $version).safe
zipfileVxx=$(printf firmware%03d $version).zip

#clean
echo cleaning
rm -rf build
rm -f $safefile $zipfileVxx $manifest $buildlog $errcodes #$safefileVxx

#clear debug flag for release build
echo clear debug flag
echo "#define NOT_FOR_FACTORY 0" > $flags_file
sleep 1 #delay for file system changes to clear cache

#keil command line opts: http://www.keil.com/support/man/docs/uv4/uv4_commandline.htm
echo building project
timeout 5.0s python error_codes_export.py $version #export error codes
Tstart=$(($(date +%s%N)/1000000))
timeout 90.0s $keil -b $project -j0 -o $buildlog
builderr=$?
Tend=$(($(date +%s%N)/1000000))
if [ ! -e $errcodes ]; then builderr=-2; fi

#restore debug flag
echo restore debug flag
echo "#define NOT_FOR_FACTORY 1" > $flags_file

#build the manifest
echo $(printf timestamp:%s "$(date +'%m/%d/%Y %H:%M:%S')") > $manifest
echo $(printf version:%d $version) >> $manifest
echo $(printf "build-time-ms:%d" $(($Tend-$Tstart))) >> $manifest
echo $(printf build-err:%d $builderr) >> $manifest
echo $(printf branch:%s $(git rev-parse --abbrev-ref HEAD)) >> $manifest
echo $(printf sha-1:%s $(git rev-parse HEAD)) >> $manifest
#if [ "$(git diff-index --cached --quiet HEAD --ignore-submodules --)" == "0" ]; then isclean=1; else isclean=0; fi
if [ "$uncomittedfilez" == "" ]; then isclean=1; else isclean=0; fi
echo $(printf working-tree-clean:%d $isclean) >> $manifest
echo $(printf working-tree-changes:%s "$uncomittedfilez") >> $manifest

#package for shipment
if [ $builderr = 0 -o $builderr = 1 ]; then #Note: keil return 1=warnings-only
  echo build: "("$builderr")" succeess in $(($Tend-$Tstart))ms
  #cp $safefile $safefileVxx
  #XXX: bootloader tools need update to support 'fixture###.safe' filenames
  #zip -9T $zipfileVxx $safefileVxx $manifest
  zip -9T $zipfileVxx $safefile $manifest $buildlog $errcodes
else
  echo build: "("$builderr")" ---FAILED--- in $(($Tend-$Tstart))ms
  rm -f $safefile #Don't leave a dirty safe laying around
fi

rm -f $manifest $buildlog $errcodes
echo done
