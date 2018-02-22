@echo OFF

where adb >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
  echo adb command not found
  exit 1
)

REM manual step - copy updated emmcdl files to release path
if not exist "emmcdl" (
  echo could not find head image directory
  exit 1
)

REM create fixture directory, if it doesn't exist
adb shell -x "mkdir -p data/local/fixture"

REM update head scripts & image files
adb push headprogram data/local/fixture/
adb shell -x "cd data/local/fixture && chmod +x headprogram"
adb shell -x "cd data/local/fixture && rm -rf emmcdl && mkdir emmcdl"
adb push emmcdl data/local/fixture/
adb push bin/emmcdl data/local/fixture/emmcdl/
adb shell -x "cd data/local/fixture/emmcdl && chmod +x emmcdl"

REM adb reboot
