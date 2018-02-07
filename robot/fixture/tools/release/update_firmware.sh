#!/bin/bash

adb=/mnt/c/android/adb.exe
if [ ! -e $adb ]; then echo $adb does not exist; exit 1; fi

#find highest version firmware package in this directory
fwver=0; fwzip="x"
for filename in *; do
  #match '###' for files named 'firmware###.zip'
  ver=$(echo $filename | grep -oP 'firmware\K([0-9]{3})?(?=\.zip)')
  len=$(echo -n $ver | wc -m)
  if [ $len -eq 3 ]; then
    if [ $ver -ge $fwver ]; then fwver=$ver; fwzip=$filename; fi
    echo "found $filename" # (v$ver)" # -> using '$fwzip' (v$fwver)"
  fi
done

if [ ! -e $fwzip ]; then echo could not find a firmware package; exit 1; fi
echo "using $fwzip ($fwver)"

#unzip the package
tempfolder=firmware
safefile=$tempfolder/fixture.safe
safefileVxx=$tempfolder/fixture$fwver.safe
rm -rf $tempfolder && mkdir $tempfolder
unzip -o $fwzip -d $tempfolder
if [ ! -e $safefile ]; then echo package does not contain a valid firmware image; exit 1; fi
cp $safefile $safefileVxx #add vers numbering to the filename - easier to track after distribution

#show the manifest
manifest=$tempfolder/manifest
if [ -e $manifest ]; then
  echo manifest:
  while read line; do 
    echo "  $line"
  done < $manifest
fi

echo "------------- untested beyond this point -----------------"
$adb devices
exit 0

#create fixture directory, if it doesn't exist
$adb shell -x "mkdir -p data/local/fixture"

#load updated helper files
$adb push dfu data/local/fixture/
$adb push $safefileVxx data/local/fixture/
$adb shell -x "cd data/local/fixture && chmod +x dfu"

#send firmware to bootloader
echo updating firmware...
$adb shell -x "pkill helper"
$adb shell -x "cd data/local/fixture && ./dfu $safefileVxx"

$adb reboot
echo done
