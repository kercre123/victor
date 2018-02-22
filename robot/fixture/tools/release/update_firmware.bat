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

REM manual step - copy updated firmware package to release path
if not exist "fixture.safe" (
  echo could not find a firmware package
  exit 1
)

REM create fixture directory, if it doesn't exist
adb shell -x "mkdir -p data/local/fixture"

REM load updated helper files
adb push dfu data/local/fixture/
adb push fixture.safe data/local/fixture/
adb shell -x "cd data/local/fixture && chmod +x dfu"

REM send firmware to bootloader
echo updating firmware...
adb shell -x "pkill helper"
adb shell -x "cd data/local/fixture && ./dfu fixture.safe"

REM echo restarting helper
echo restarting helper
adb shell "/data/local/fixture/helper > /dev/null 2>&1 &"

echo ----- please power cycle the fixture (do NOT use adb reboot!) -----
