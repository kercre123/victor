@echo OFF

where adb >nul 2>nul
if %ERRORLEVEL% NEQ 0 ( echo "adb command not found" & goto :END )

echo waiting for device connection...
adb wait-for-device && adb shell "systemctl stop anki-robot.target && systemctl disable anki-robot.target && sync"
adb shell "echo shell connection established"
if %ERRORLEVEL% NEQ 0 ( echo adb not connected to a device e=%ERRORLEVEL% & goto :END )

echo stopping helper process...
adb shell "echo ps old: `ps | grep ./helper | grep -v grep`"
adb shell "killall -9 helper && sleep 1 && ps | grep ./helper | grep -v grep"

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

:END
pause
