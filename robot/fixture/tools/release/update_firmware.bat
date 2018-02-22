@echo OFF

where adb >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
  echo adb command not found
  exit 1
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
echo done

REM adb reboot
