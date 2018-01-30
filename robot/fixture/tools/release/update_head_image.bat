@echo OFF

REM 'adb.exe' expected to be in C:\Users\[username]\android\adb.exe
set ADB_PATH=%userprofile%\android
if not exist "%ADB_PATH%\adb.exe" (
  echo %ADB_PATH%\adb.exe does not exist
  goto END
)

REM manual step - copy updated emmcdl files to release path
if not exist "emmcdl" (
  echo could not find head image directory
  goto END
)

REM create fixture directory, if it doesn't exist
%ADB_PATH%\adb shell -x "mkdir -p data/local/fixture"

REM update head scripts & image files
%ADB_PATH%\adb push headprogram data/local/fixture/
%ADB_PATH%\adb shell -x "cd data/local/fixture && chmod +x headprogram"
%ADB_PATH%\adb shell -x "cd data/local/fixture && rm -rf emmcdl && mkdir emmcdl"
%ADB_PATH%\adb push emmcdl data/local/fixture/
%ADB_PATH%\adb push bin/emmcdl data/local/fixture/emmcdl/
%ADB_PATH%\adb shell -x "cd data/local/fixture/emmcdl && chmod +x emmcdl"

REM %ADB_PATH%\adb reboot
:END
