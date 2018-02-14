@echo OFF

set ADB=C:\android\adb.exe
if not exist "%ADB%" (
  echo %ADB% does not exist
  goto END
)

REM create fixture directory, if it doesn't exist
%ADB% shell -x "mkdir -p data/local/fixture"

REM load updated helper files
%ADB% push helper data/local/fixture/
%ADB% push helpify data/local/fixture/
%ADB% push display data/local/fixture/
%ADB% shell -x "cd data/local/fixture && chmod +x helper"
%ADB% shell -x "cd data/local/fixture && chmod +x helpify"
%ADB% shell -x "cd data/local/fixture && chmod +x display"

REM enable helper auto-start
%ADB% shell -x "cd data/local/fixture && ./helpify"

%ADB% reboot
:END
