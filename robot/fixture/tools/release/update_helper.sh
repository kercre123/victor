#!/bin/sh

echo Install files...
ssh root@${ROBOT_IP} mkdir -p /data/local/fixture
scp helper root@${ROBOT_IP}:/data/local/fixture
scp helpify root@${ROBOT_IP}:/data/local/fixture
scp helpify root@${ROBOT_IP}:/data/local/fixture
ssh root@${ROBOT_IP} "cd /data/local/fixture && chmod +x * && dos2unix helpify"

echo Enabling helper mode...

ssh root@${ROBOT_IP} "mount -o exec,remount /data"
ssh root@${ROBOT_IP} "mount -o remount,rw /"
ssh root@${ROBOT_IP} "cd /data/local/fixture && ./helpify"

echo Rebooting...
ssh root@${ROBOT_IP} "sync && /sbin/reboot"
