#!/system/bin/sh
LOG=/dev/null
killall -9 bootanimation
echo 800000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
echo 1 > /sys/kernel/debug/regulator/8916_l10/enable
echo 1 > /sys/kernel/debug/regulator/8916_l8/enable
# NDM-XXX: Horrible hack to force on camera on P2 - please complain when you notice this
echo 994 > /sys/class/gpio/export
echo out > /sys/class/gpio/gpio994/direction
echo 1 > /sys/class/gpio/gpio994/value
# End camera force hack
echo 936 > /sys/class/gpio/export # GPIO 25
echo 947 > /sys/class/gpio/export # GPIO 36
echo out > /sys/class/gpio/gpio936/direction
echo out > /sys/class/gpio/gpio947/direction
for B in 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0
do
    echo $B > /sys/class/gpio/gpio947/value
    seep 2
done
echo 1 > /sys/class/gpio/gpio936/value
insmod /system/lib/modules/wlan.ko
setprop ctl.stop zygote
setprop ctl.stop ppd
for B in 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0
do
    echo $B > /sys/class/gpio/gpio947/value
    seep 2
done
while ! wpa_supplicant -iwlan0 -Dnl80211 -c/system/etc/wpa_supplicant_anki.conf -O/data/misc/wifi/sockets -B
do
    echo 0 > /sys/class/gpio/gpio936/value
    sleep 2
    echo 1 > /sys/class/gpio/gpio936/value
    sleep 2
done
wpa_cli reassociate
if test -e /data/local/ipconf.sh
then
    /system/bin/sh /data/local/ipconf.sh
    echo 1 > /sys/class/gpio/gpio947/value
else
    for B in 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0
    do
	echo $B > /sys/class/gpio/gpio947/value
	sleep 1
    done
    if dhcptool wlan0
    then
	echo 1 > /sys/class/gpio/gpio947/value
    fi
fi
