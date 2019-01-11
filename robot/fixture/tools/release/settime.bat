@echo OFF

REM usage: settime.bat (serial) (adb)
set IFSERIAL=0
set IFADB=0
IF /I "%~1"=="serial" set IFSERIAL=1
IF /I "%~2"=="serial" set IFSERIAL=1
IF /I "%~1"=="adb" set IFADB=1
IF /I "%~2"=="adb" set IFADB=1

REM get the local UTC time (current PC clock)
setlocal
call :GetUnixTime UNIX_UTC
call :GetUnixTimeLocal UNIX_LOCAL
echo UTC time, GMT  : %UNIX_UTC%
echo UTC time, local: %UNIX_LOCAL%

REM METHOD1: spam all available com ports (direct serial connection to fixture)
IF %IFSERIAL% NEQ 1 goto :LENDSERIAL
powershell -ExecutionPolicy Unrestricted -Command .\settime.ps1 -unixlocal %UNIX_LOCAL%
:LENDSERIAL

REM METHOD2: bridge through helper head to send the command
IF %IFADB% NEQ 1 goto :LENDADB

where adb >nul 2>nul
if %ERRORLEVEL% NEQ 0 ( echo "adb command not found" & goto :END )

adb shell "echo shell connection established"
if %ERRORLEVEL% NEQ 0 ( echo adb not connected to a device e=%ERRORLEVEL% & goto :END )

echo hijack ttyHS0
adb shell "killall -9 helper && sleep 1 && ps | grep ./helper | grep -v grep"
adb shell "stty -F /dev/ttyHS0 1000000"

adb shell "echo settime %UNIX_LOCAL%"
adb shell "echo settime %UNIX_LOCAL% > /dev/ttyHS0"

REM READ REPLY
REM XXX no worky :(
REM adb shell "end=$(date +%s)+2; echo $end..$((end)); while [ $((date +%s)) -lt $((end)) ]; do cat </dev/tty; done"

REM echo restarting helper process...
REM adb shell "echo `cd /data/local/fixture && nohup ./helper >/dev/null 2>&1 </dev/null &` >/dev/null && sleep 1"
REM adb shell "echo ps new: `ps | grep ./helper | grep -v grep` && sleep 1"

echo rebooting...
adb shell "sync && sleep 1"
adb reboot

echo ----- Done! Verify display time after reboot -----
:LENDADB


REM pause
goto :EOF


REM ============================ PC Time Subroutines ====================================

:GetUnixTimeLocal
setlocal enableextensions
for /f %%x in ('wmic path win32_localtime get /format:list ^| findstr "="') do (
  set %%x)
set /a z=(14-100%Month%%%100)/12, y=10000%Year%%%10000-z
set /a ut=y*365+y/4-y/100+y/400+(153*(100%Month%%%100+12*z-3)+2)/5+Day-719469
set /a ut=ut*86400+100%Hour%%%100*3600+100%Minute%%%100*60+100%Second%%%100
endlocal & set "%1=%ut%" & goto :EOF

REM https://stackoverflow.com/questions/11385030/batch-timestamp-to-unix-time
:GetUnixTime
setlocal enableextensions
for /f %%x in ('wmic path win32_utctime get /format:list ^| findstr "="') do (
  set %%x)
set /a z=(14-100%Month%%%100)/12, y=10000%Year%%%10000-z
set /a ut=y*365+y/4-y/100+y/400+(153*(100%Month%%%100+12*z-3)+2)/5+Day-719469
set /a ut=ut*86400+100%Hour%%%100*3600+100%Minute%%%100*60+100%Second%%%100
endlocal & set "%1=%ut%" & goto :EOF

:END
:EOF

