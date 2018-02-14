@echo OFF

set ADB=C:\android\adb.exe
if not exist "%ADB%" (
  echo %ADB% does not exist
  goto END
)

REM manual step - copy updated firmware package to release path
if not exist "fixture.safe" (
  echo could not find a firmware package
  goto END
)

REM create fixture directory, if it doesn't exist
%ADB% shell -x "mkdir -p data/local/fixture"

REM load updated helper files
%ADB% push dfu data/local/fixture/
%ADB% push fixture.safe data/local/fixture/
%ADB% shell -x "cd data/local/fixture && chmod +x dfu"

REM send firmware to bootloader
echo updating firmware...
%ADB% shell -x "pkill helper"
%ADB% shell -x "cd data/local/fixture && ./dfu fixture.safe"

%ADB% reboot
:END
