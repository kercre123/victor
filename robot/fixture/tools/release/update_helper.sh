#!/bin/sh

set -e

echo waiting for device connection...
adb wait-for-device && adb shell "systemctl stop anki-robot.target && systemctl disable anki-robot.target && sync"
adb shell "echo shell connection established"

echo stop helper process...
adb shell "pkill helper && sleep 1"
#REM adb shell "ps | grep helper"

echo make filesystem r/w
adb shell "mount -o remount,rw /"

echo load updated helper files...
adb shell -x "mkdir -p /data/local/fixture"
adb push helper /data/local/fixture/
adb push helpify /data/local/fixture/
adb shell -x "cd /data/local/fixture && chmod +x * && dos2unix helpify"

echo enable helper auto-boot...
adb shell "mount -o exec,remount /data"
adb shell -x "cd /data/local/fixture && ./helpify"

echo rebooting...
adb shell "sync && sleep 1"
adb reboot

echo Done!
