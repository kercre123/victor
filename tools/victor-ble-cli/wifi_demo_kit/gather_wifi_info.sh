#!/usr/bin/env bash
wifi_count=0

adb shell wpa_cli scan
sleep 2
wifi_count="$(adb shell wpa_cli scan_result | awk '{count++} END {print count}')"
printf "\n\nWifi count is $wifi_count\n\n"

printf "Detailed list of Wifi\n\n"
adb shell wpa_cli scan_result	
printf "\n\nBegining connection to $robot_id \n\n"	

