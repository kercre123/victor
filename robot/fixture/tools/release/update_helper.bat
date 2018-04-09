@echo OFF

where adb >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
  echo adb command not found
  exit 1
)

echo waiting for device connection...
adb wait-for-device && adb shell "systemctl stop anki-robot.target && systemctl disable anki-robot.target && sync"
adb shell "echo shell connection established"
if %ERRORLEVEL% NEQ 0 (
  echo adb not connected to a device, e=%ERRORLEVEL%
  exit 2
)

echo stop helper process...
adb shell "pkill helper && sleep 1"
REM adb shell "ps | grep helper"

REM create fixture directory, if it doesn't exist
adb shell -x "mkdir -p data/local/fixture && sync"

echo load updated helper files...
adb push helper /data/local/fixture/
adb push helpify /data/local/fixture/
adb push display /data/local/fixture/
adb shell -x "cd /data/local/fixture && chmod +x helper"
adb shell -x "cd /data/local/fixture && chmod +x helpify && dos2unix helpify"
adb shell -x "cd /data/local/fixture && chmod +x display"
adb shell "sync"

REM enable helper auto-start
adb shell "mount -o exec,remount /data"
adb shell -x "cd /data/local/fixture && ./helpify -d"
adb shell "sync"

REM echo -------- POWER CYCLE NOW --------
echo rebooting
adb reboot
pause

