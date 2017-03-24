#!/bin/bash
# Copyright (c) 2014 Anki, Inc.
# All rights reserved.

adb='adb'
#shift
args=$@

GREEN="\033[1;32m"
NORMAL="\033[0m"

echo -e "${GREEN}usage: $0 flags logcat-filter-1 logcat-filter-2 ... logcat-filter-N"
echo -e "usage (All Messages): $0 -a${NORMAL}"


HELP=0
ALL=0

while getopts ":ah" opt; do
    case $opt in
        a)
            ALL=1
            ;;
        h)
            HELP=1
            ;;
    esac
done


if [ "$#" == "0" ]; then
  args="-s -v threadtime Unity:I das:D dasJava:V libc:I DEBUG:D AndroidRuntime:W fmod:V BLEPeripheral:V VehicleConnection:D WifiMessagePort:V HockeyApp:V BackgroundConnectivity:D AnkiUtil:V AudioCaptureSystem:V"
  echo using default filter: $args
elif [ $ALL -eq 1 ]; then
  args="-v threadtime *:V"
  echo using default filter: $args
elif [ $HELP -eq 1 ]; then
  exit
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
