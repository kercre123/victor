#!/bin/bash
# Copyright (c) 2014 Anki, Inc.
# All rights reserved.

adb='adb'
#shift
args=$@

if [ "$#" == "0" ]; then
  echo usage: $0 logcat-filter-1 logcat-filter-2 ... logcat-filter-N
  args="-s -v threadtime Unity:I das:D dasJava:V libc:I DEBUG:D AndroidRuntime:W fmod:V BLEPeripheral:V VehicleConnection:D WifiMessagePort:V HockeyApp:V BackgroundConnectivity:D AnkiUtil:V"
  echo usign default filter: $args
fi

commonStart='
tell application "Terminal"
activate
do script 
'

commandStart='
my makeTab()
do script "'

commandEnd1='" in tab '
commandEnd2=' of front window'

commonEnd='
end tell

on makeTab()
tell application "System Events" to keystroke "t" using {command down}
delay 0.2
end makeTab
'



devices=""
lines=`$adb devices`
IFS=$'\n'
for line in $lines
do
  if [ ! "$line" = "" ] && [ `echo $line | awk '{print $2}'` = "device" ]; then
    # do work
    device=`echo $line | awk '{print $1}'`
    devices="$devices $device"
  fi
done



/bin/mkdir -p ~/logcats
ts=`/bin/date +%y%m%d-%H.%M`
logcatfile="~/logcats/$(basename $0).$ts"
file="/tmp/$(basename $0).tmp"
echo "$commonStart" > $file
IFS=$' '
counter=1
for device in $devices; do
  echo "${commandStart}clear;osascript -e 'tell application \\\"System Events\\\" to keystroke \\\"k\\\" using {command down, option down}';adb -s $device logcat -c;adb -s $device logcat $args | /usr/bin/tee $logcatfile.$device.txt ${commandEnd1}$counter${commandEnd2}" >> $file
  counter=$((counter + 1))
done
echo "$commonEnd" >> $file

echo osascript $file
/usr/bin/osascript $file
