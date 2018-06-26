@echo OFF

setlocal
call :GetUnixTime UNIX_UTC
call :GetUnixTimeLocal UNIX_LOCAL
echo UTC time, GMT  : %UNIX_UTC%
echo UTC time, local: %UNIX_LOCAL%

REM METHOD1: spam all available com ports (direct serial connection to fixture)
powershell -ExecutionPolicy Unrestricted -Command .\settime.ps1 -unixlocal %UNIX_LOCAL%

REM METHOD2: bridge through helper head to send the command
REM XXX

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

:EOF

