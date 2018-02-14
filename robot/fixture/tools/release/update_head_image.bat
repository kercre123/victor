@echo OFF

set ADB=C:\android\adb.exe
if not exist "%ADB%" (
  echo %ADB% does not exist
  goto END
)

REM manual step - copy updated emmcdl files to release path
if not exist "emmcdl" (
  echo could not find head image directory
  goto END
)

REM create fixture directory, if it doesn't exist
%ADB% shell -x "mkdir -p data/local/fixture"

REM update head scripts & image files
%ADB% push headprogram data/local/fixture/
%ADB% shell -x "cd data/local/fixture && chmod +x headprogram"
%ADB% shell -x "cd data/local/fixture && rm -rf emmcdl && mkdir emmcdl"
%ADB% push emmcdl data/local/fixture/
%ADB% push bin/emmcdl data/local/fixture/emmcdl/
%ADB% shell -x "cd data/local/fixture/emmcdl && chmod +x emmcdl"

REM %ADB% reboot
:END
