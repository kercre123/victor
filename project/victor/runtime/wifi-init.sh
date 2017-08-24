#!/system/bin/sh
LOG=/data/local/tmp/wifi-init.$$.log

IP_ADDRESS="192.168.1.55"

echo "stop zygote"
setprop ctl.stop zygote  > $LOG 2>&1
setprop ctl.stop ppd >> $LOG 2>&1
killall -9 bootanimation >> $LOG 2>&1

echo "load wlan kernel mod"
insmod /system/lib/modules/wlan.ko >> $LOG 2>&1
sleep 5

echo "start wpa_supplicant"
WPA_PID=`pidof wpa_supplicant`
if [ $? -eq 0 ]; then
  kill $WPA_PID
fi

wpa_supplicant -iwlan0 -Dnl80211 -c/data/data/com.anki.cozmoengine/config/wpa_supplicant_ankitest2.conf -O/data/misc/wifi/sockets -B >> $LOG 2>&1

echo "associate with Wi-Fi access point"
wpa_cli reassociate >> $LOG 2>&1
sleep 10

#echo "configure static IP"
#ifconfig wlan0 ${IP_ADDRESS} netmask 255.255.255.0

echo "obtaining IP address"
dhcptool wlan0  >> $LOG 2>&1

echo "power serial port"
echo 1 > /sys/kernel/debug/regulator/8916_l10/enable >> $LOG 2>&1
