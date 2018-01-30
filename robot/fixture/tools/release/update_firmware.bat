@echo OFF

REM 'adb.exe' expected to be in C:\Users\[username]\android\adb.exe
set ADB_PATH=%userprofile%\android
if not exist "%ADB_PATH%\adb.exe" (
  echo %ADB_PATH%\adb.exe does not exist
  goto END
)

REM manual step - copy updated firmware package to release path
if not exist "fixture.safe" (
  echo could not find a firmware package
  goto END
)

REM create fixture directory, if it doesn't exist
%ADB_PATH%\adb shell -x "mkdir -p data/local/fixture"

REM load updated helper files
%ADB_PATH%\adb push dfu data/local/fixture/
%ADB_PATH%\adb push fixture.safe data/local/fixture/
%ADB_PATH%\adb shell -x "cd data/local/fixture && chmod +x dfu"

REM send firmware to bootloader
echo updating firmware...
%ADB_PATH%\adb shell -x "pkill helper"
%ADB_PATH%\adb shell -x "cd data/local/fixture && ./dfu fixture.safe"

%ADB_PATH%\adb reboot
:END
