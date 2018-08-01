@echo OFF

where adb >nul 2>nul
if %ERRORLEVEL% NEQ 0 ( echo "adb command not found" & goto :END )

adb shell "echo shell connection established"
if %ERRORLEVEL% NEQ 0 ( echo adb not connected to a device e=%ERRORLEVEL% & goto :END )

rd /S /Q logs

echo stopping helper process...
adb shell "echo ps old: `ps | grep ./helper | grep -v grep`"
adb shell "killall -9 helper && sleep 1 && ps | grep ./helper | grep -v grep"

echo reading logs...
adb pull data/local/fixture/logs
if %ERRORLEVEL% == 0 (
  echo deleting logs from the fixture
  adb shell "rm -rf /data/local/fixture/logs && sync"
  
  REM echo restarting helper process...
  REM adb shell "echo `cd /data/local/fixture && nohup ./helper >/dev/null 2>&1 </dev/null &` >/dev/null && sleep 1"
  REM adb shell "echo ps new: `ps | grep ./helper | grep -v grep` && sleep 1"
  
  echo rebooting && adb reboot
) else (
  echo -----FAILED TO GET FIXTURE LOGS-------
  echo rebooting && adb reboot
)

:END
