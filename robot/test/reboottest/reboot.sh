set -x
counter=0
while true
do
    adb wait-for-device
    sleep 10
    adb shell "systemctl stop victor.target && mount -o remount,rw,exec /var/volatile"
    adb pull /dev/rampost.log logs/rampost.${counter}.log
    adb push new.syscon.dfu /tmp
    adb push mp.syscon.dfu /tmp
    adb push rampost /tmp
    sleep 10
    adb shell "/tmp/rampost -f /tmp/new.syscon.dfu"
    sleep 10
    adb shell "/tmp/rampost -f /tmp/mp.syscon.dfu"
    sleep 10
    counter=$((counter+1))
    adb shell reboot
done    

