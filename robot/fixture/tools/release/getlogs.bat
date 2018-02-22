@echo OFF

where adb >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
  echo adb command not found
  exit 1
)

adb pull data/local/fixture/logs
