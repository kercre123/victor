@echo OFF

REM 'adb.exe' expected to be in C:\Users\[username]\android\adb.exe
set ADB_PATH=%userprofile%\android
if not exist "%ADB_PATH%\adb.exe" (
  echo %ADB_PATH%\adb.exe does not exist
  goto END
)

%ADB_PATH%\adb pull data/local/fixture/logs

:END
