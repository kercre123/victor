#!/bin/bash
# Copyright (c) 2014 Anki, Inc.
# All rights reserved.

GIT=`which git`
if [ -z $GIT ]
then
    echo git not found
    exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

ADB=adb
if [ -e $TOPLEVEL/local.properties ]; then
    ANDROID_HOME=`egrep sdk.dir $TOPLEVEL/local.properties | awk -F= '{print $2;}'`
    ADB=$ANDROID_HOME/platform-tools/adb
    if [ ! -x $ADB ]; then
        ADB=adb
    fi
fi

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
  args="-s -v threadtime Unity:I das:D dasJava:V libc:I DEBUG:D AndroidRuntime:W fmod:V BLEPeripheral:V VehicleConnection:D WifiMessagePort:V HockeyApp:V BackgroundConnectivity:D AnkiUtil:V AudioCaptureSystem:V AudioSystem.Logging:V AudioSystem.Debugging:V"
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
lines=`$ADB devices`
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
  echo "${commandStart}clear;osascript -e 'tell application \\\"System Events\\\" to keystroke \\\"k\\\" using {command down, option down}';$ADB -s $device logcat -c;$ADB -s $device logcat $args | /usr/bin/tee $logcatfile.$device.txt ${commandEnd1}$counter${commandEnd2}" >> $file
  counter=$((counter + 1))
done
echo "$commonEnd" >> $file

echo osascript $file
/usr/bin/osascript $file
