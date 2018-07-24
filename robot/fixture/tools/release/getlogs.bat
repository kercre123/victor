@echo OFF

where adb >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
  echo adb command not found
  exit 1
)

adb shell "echo shell connection established"
if %ERRORLEVEL% NEQ 0 (
  echo adb not connected to a device, e=%ERRORLEVEL%
  exit 2
)

rd /S /Q logs
adb pull data/local/fixture/logs
if %ERRORLEVEL% == 0 (
  echo deleting logs from the fixture
  adb shell "rm -rf /data/local/fixture/logs && sync"
  adb reboot
) else (
  echo -----FAILED TO GET FIXTURE LOGS-------
)

