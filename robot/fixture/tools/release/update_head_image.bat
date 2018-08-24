@echo OFF
rem SETLOCAL ENABLEDELAYEDEXPANSION

REM usage: update_head_image.bat (patch) (bin)
REM image is NOT updated if patch and/or bin are specified
REM image+patch+bin updated by default, if no args
set IFIMAGE=1 & set IFPATCH=0 & set IFBIN=0 & set ISDEBUG=0 & set argCount=0
for %%x in (%*) do (
  set /A argCount+=1
  IF /I "%%~x"=="debug" ( set ISDEBUG=1 )
  IF /I "%%~x"=="patch" ( set IFIMAGE=0 & set IFPATCH=1 )
  IF /I "%%~x"=="bin"   ( set IFIMAGE=0 & set IFBIN=1 )
  IF %ISDEBUG%==1 ( echo "arg %argCount% = %%~x" )
)
IF %IFIMAGE%==1 ( set IFPATCH=1 & set IFBIN=1 )
IF %ISDEBUG%==1 ( echo image=%IFIMAGE% patch=%IFPATCH% bin=%IFBIN% )

where adb >nul 2>nul
if %ERRORLEVEL% NEQ 0 ( echo "adb command not found" & goto :END )

adb shell "echo shell connection established"
if %ERRORLEVEL% NEQ 0 ( echo adb not connected to a device e=%ERRORLEVEL% & goto :END )

adb shell "echo 1094400 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq && sleep 1"
REM adb shell "echo cpu clock $(cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq)"
adb shell -x "mkdir -p /data/local/fixture"

IF %IFIMAGE%==1 ( 
  echo updating head image...
  if not exist "emmcdl" (
    echo ERROR: could not find head image directory
  ) else (
    adb shell -x "rm -rf /data/local/fixture/emmcdl/ && sync && sleep 1 && mkdir /data/local/fixture/emmcdl/"
    adb push emmcdl data/local/fixture/
  )
)

IF %IFBIN%==1 ( 
  echo updating binaries...
  adb push headprogram /data/local/fixture/
  adb push usbserial.ko /data/local/fixture/
  adb push makebc /data/local/fixture/
  adb shell "chmod +x /data/local/fixture/*"
  adb shell "dos2unix /data/local/fixture/headprogram"
  adb push bin/emmcdl /data/local/fixture/emmcdl/
  adb shell -x "cd /data/local/fixture/emmcdl && chmod +x emmcdl"
)

set TESTFILE=noname
IF %IFPATCH%==1 ( 
  if exist "emmcdl-p1" (
    echo applying image patch1...
    for %%f in ("emmcdl-p1/*") do (
      rem echo %%f , %%~nxf
      adb push "emmcdl-p1/%%f" data/local/fixture/emmcdl
    )
  ) else (
    echo no image patches detected
  )
)

adb shell "sync"

echo restarting helper process...
adb shell "echo ps old: `ps | grep ./helper | grep -v grep`"
adb shell "killall -9 helper && sleep 1 && ps | grep ./helper | grep -v grep"
adb shell "echo `cd /data/local/fixture && nohup ./helper >/dev/null 2>&1 </dev/null &` >/dev/null && sleep 1"
adb shell "echo ps new: `ps | grep ./helper | grep -v grep`"

echo Done!

:END
REM pause
