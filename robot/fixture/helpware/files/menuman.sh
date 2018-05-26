#!/bin/sh

## Enter FCC test mode - WiFi will NOT work on this robot                                           
rmmod wlan.ko                                                                                       
insmod /usr/lib/modules/3.18.66-perf/extra/wlan.ko con_mode=5                                            
iwpriv wlan0 ftm 1                                                                                  
iwpriv wlan0 ena_chain 2

cd /anki/menuman
while true; do
sleep 3
./menuman
done
