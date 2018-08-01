@echo OFF

where adb >nul 2>nul
if %ERRORLEVEL% NEQ 0 ( echo "adb command not found" & goto :END )

adb shell "echo shell connection established"
if %ERRORLEVEL% NEQ 0 ( echo adb not connected to a device e=%ERRORLEVEL% & goto :END )

if not exist "emmcdl" (
  echo could not find head image directory
  exit 1
)

echo update head scripts and image files...
adb shell "echo 1094400 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq && sleep 1"
REM adb shell "echo cpu clock $(cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq)"
adb shell -x "mkdir -p /data/local/fixture"
adb push headprogram /data/local/fixture/
adb push usbserial.ko /data/local/fixture/
adb push makebc /data/local/fixture/
adb shell "chmod +x /data/local/fixture/*"
adb shell "dos2unix /data/local/fixture/headprogram"
adb shell -x "rm -rf /data/local/fixture/emmcdl/ && sync && sleep 1 && mkdir /data/local/fixture/emmcdl/"
adb push emmcdl data/local/fixture/
adb push bin/emmcdl /data/local/fixture/emmcdl/
adb shell -x "cd /data/local/fixture/emmcdl && chmod +x emmcdl"
adb shell "sync"

echo restarting helper process...
adb shell "echo ps old: `ps | grep ./helper | grep -v grep`"
adb shell "killall -9 helper && sleep 1 && ps | grep ./helper | grep -v grep"
adb shell "echo `cd /data/local/fixture && nohup ./helper >/dev/null 2>&1 </dev/null &` >/dev/null && sleep 1"
adb shell "echo ps new: `ps | grep ./helper | grep -v grep`"

echo Done!

:END
pause
