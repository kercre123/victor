@echo OFF

where adb >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
  echo adb command not found
  exit 1
)

adb shell "echo shell connection established"
if %ERRORLEVEL% NEQ 0 (
  echo adb not connected to a device, e=%ERRORLEVEL%
  exit 2
)

if not exist "emmcdl" (
  echo could not find head image directory
  exit 1
)

REM create fixture directory, if it doesn't exist
adb shell -x "mkdir -p data/local/fixture && sync"

REM update head scripts & image files
adb push headprogram data/local/fixture/
adb push usbserial.ko data/local/fixture/
adb shell "chmod +x /data/local/fixture/*"
adb shell -x "rm -rf /data/local/fixture/emmcdl/ && sync && sleep 1 && mkdir /data/local/fixture/emmcdl/"
adb push emmcdl data/local/fixture/
adb push bin/emmcdl data/local/fixture/emmcdl/
adb shell -x "cd data/local/fixture/emmcdl && chmod +x emmcdl"
adb shell "sync"

echo restart helper process...
adb shell -x "pkill helper && sleep 1"
adb shell "/data/local/fixture/helper > /dev/null 2>&1 &"

echo Done!
pause
