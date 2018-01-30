@echo OFF

REM 'adb.exe' expected to be in C:\Users\[username]\android\adb.exe
set ADB_PATH=%userprofile%\android
if not exist "%ADB_PATH%\adb.exe" (
  echo %ADB_PATH%\adb.exe does not exist
  goto END
)

REM create fixture directory, if it doesn't exist
%ADB_PATH%\adb shell -x "mkdir -p data/local/fixture"

REM load updated helper files
%ADB_PATH%\adb push helper data/local/fixture/
%ADB_PATH%\adb push helpify data/local/fixture/
%ADB_PATH%\adb push display data/local/fixture/
%ADB_PATH%\adb shell -x "cd data/local/fixture && chmod +x helper"
%ADB_PATH%\adb shell -x "cd data/local/fixture && chmod +x helpify"
%ADB_PATH%\adb shell -x "cd data/local/fixture && chmod +x display"

REM enable helper auto-start
%ADB_PATH%\adb shell -x "cd data/local/fixture && ./helpify"

%ADB_PATH%\adb reboot
:END
