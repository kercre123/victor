@echo OFF

where adb >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
  echo adb command not found
  exit 1
)

REM create fixture directory, if it doesn't exist
adb shell -x "mkdir -p data/local/fixture"

REM load updated helper files
adb push helper data/local/fixture/
adb push helpify data/local/fixture/
adb push display data/local/fixture/
adb shell -x "cd data/local/fixture && chmod +x helper"
adb shell -x "cd data/local/fixture && chmod +x helpify"
adb shell -x "cd data/local/fixture && chmod +x display"

REM enable helper auto-start
adb shell -x "cd data/local/fixture && ./helpify"

adb reboot
