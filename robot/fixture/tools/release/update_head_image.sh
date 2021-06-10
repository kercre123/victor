#!/bin/sh

ROBOT_IP=192.168.65.76

ssh root@$ROBOT_IP "echo 1094400 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq && sleep 1"
ssh root@$ROBOT_IP "mkdir -p /data/local/fixture"
scp headprogram root@$ROBOT_IP:/data/local/fixture/
scp usbserial.ko root@$ROBOT_IP:/data/local/fixture/
scp makebc root@$ROBOT_IP:/data/local/fixture/
ssh root@$ROBOT_IP "chmod +x /data/local/fixture/*"
ssh root@$ROBOT_IP "dos2unix /data/local/fixture/headprogram"
ssh root@$ROBOT_IP "rm -rf /data/local/fixture/emmcdl/ && sync && sleep 1 && mkdir /data/local/fixture/emmcdl/"
scp bin/emmcdl root@$ROBOT_IP:/data/local/fixture/emmcdl/
ssh root@$ROBOT_IP "cd /data/local/fixture/emmcdl && chmod +x emmcdl"
ssh root@$ROBOT_IP "sync"

echo restarting helper...
ssh root@$ROBOT_IP "pkill helper && sleep 1"
ssh root@$ROBOT_IP "cd /data/local/fixture && ./helper > /dev/null 2>&1 < /dev/null &"
ssh root@$ROBOT_IP "sleep 1 && ps | grep helper"

echo Done!
