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

REM caller specify a firmware .safe file?
set ARG1=%~1
if "%ARG1%" NEQ "" (
  set FILEPATH=%ARG1%
  set FILENAME=%~n1%~x1
) else (
  set FILEPATH=fixture.safe
  set FILENAME=fixture.safe
)
if not exist %FILEPATH% (
  echo could not find firmware file: %FILEPATH%
  exit 1
)
echo update firmware to: %FILEPATH% (%FILENAME%)

echo load dfu files...
adb shell -x "mkdir -p /data/local/fixture"
adb push dfu data/local/fixture/
adb push %FILEPATH% data/local/fixture/
adb shell -x "cd data/local/fixture && chmod +x dfu"

echo updating firmware...
adb shell -x "pkill helper && sleep 1"
adb shell -x "cd data/local/fixture && ./dfu %FILENAME%"

echo restarting helper...
adb shell "sleep 1 && cd /data/local/fixture && ./helper > /dev/null 2>&1 < /dev/null &"
adb shell "sleep 1 && ps | grep helper"

REM echo rebooting...
adb shell "sync && sleep 1"
REM adb reboot

echo Done!
pause
