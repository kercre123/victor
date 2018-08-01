@echo OFF

where adb >nul 2>nul
if %ERRORLEVEL% NEQ 0 ( echo "adb command not found" & goto :END )

adb shell "echo shell connection established"
if %ERRORLEVEL% NEQ 0 ( echo adb not connected to a device e=%ERRORLEVEL% & goto :END )

set FILENAME=getstats.log
set OUTFILE=/tmp/%FILENAME%
del /F /Q %FILENAME%
adb shell "rm %OUTFILE%"

echo collecting stats...
REM adb shell "echo helper pid `ps | grep ./helper | grep -v grep | awk '{print $1}'`"
REM top -m -n 1 -p `ps | grep ./helper | grep -v grep | awk '{print $1}'`
adb shell "echo 'top -n 1' >> %OUTFILE% && top -n 1 >> %OUTFILE% && echo '' >> %OUTFILE%"
adb shell "echo 'df /data' >> %OUTFILE% && df /data >> %OUTFILE% && echo '' >> %OUTFILE%"
adb shell "echo 'du /data/local/fixture' >> %OUTFILE% && du /data/local/fixture >> %OUTFILE% && echo '' >> %OUTFILE%"

echo restarting helper process...
adb shell "echo '--------- RESTART HELPER PROCESS -------------' >> %OUTFILE% && echo '' >> %OUTFILE%"
adb shell "echo ps old: `ps | grep ./helper | grep -v grep`"
adb shell "killall -9 helper && sleep 1 && ps | grep ./helper | grep -v grep"
adb shell "echo `cd /data/local/fixture && nohup ./helper >/dev/null 2>&1 </dev/null &` >/dev/null && sleep 1"
adb shell "echo ps new: `ps | grep ./helper | grep -v grep`"

echo collecting stats...
adb shell "sleep 5"
adb shell "echo 'top -n 1' >> %OUTFILE% && top -n 1 >> %OUTFILE% && echo '' >> %OUTFILE%"
adb shell "echo 'df /data' >> %OUTFILE% && df /data >> %OUTFILE% && echo '' >> %OUTFILE%"
adb shell "echo 'du /data/local/fixture' >> %OUTFILE% && du /data/local/fixture >> %OUTFILE% && echo '' >> %OUTFILE%"

echo rebooting helper head...
adb shell "echo '--------- REBOOT HELPER HEAD -------------' >> %OUTFILE% && echo '' >> %OUTFILE%"
adb shell "sync" && adb reboot && adb wait-for-device
echo delay for stable cpu usage && adb shell "sleep 10"

echo collecting stats...
adb shell "echo 'top -n 1' >> %OUTFILE% && top -n 1 >> %OUTFILE% && echo '' >> %OUTFILE%"
adb shell "echo 'df /data' >> %OUTFILE% && df /data >> %OUTFILE% && echo '' >> %OUTFILE%"
adb shell "echo 'du /data/local/fixture' >> %OUTFILE% && du /data/local/fixture >> %OUTFILE% && echo '' >> %OUTFILE%"

adb shell "sync"
adb pull %OUTFILE%

:END
pause
exit 0
