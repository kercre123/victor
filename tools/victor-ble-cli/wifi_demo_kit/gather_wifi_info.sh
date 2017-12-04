#!/usr/bin/env bash

if [[ "$1" == *ist ]]; then
	wifi_count=0
	
	adb shell wpa_cli scan 
	sleep 2
	wifi_count="$(adb shell wpa_cli scan_result)"
	printf "\n\nWifi count is $wifi_count\n\n" >> count.log
	
	printf "Detailed list of Wifi\n\n"
	
	adb shell wpa_cli scan_result | awk '{print $5}'
	exit
elif [[ $# > 2 ]]; then
	robot_id=$1
	SSID=$2 
	password=$3
	current_time=`date '+%Y-%m-%d-%H.%M.%S'`
	wifi_count=0
	robot_id_log="$robot_id-$current_time.log"
	touch $robot_id_log
	
	adb shell wpa_cli scan > $robot_id_log 2>&1 
	sleep 2
	wifi_count="$(adb shell wpa_cli scan_result | awk '{count++} END {print count}')"
	printf "\n\nWifi count is $wifi_count\n\n" >> $robot_id_log
	
	printf "Detailed list of Wifi\n\n" >> $robot_id_log
	adb shell wpa_cli scan_result >> $robot_id_log 2>&1 	
	printf "\n\nBegining connection to $robot_id \n\n" >> $robot_id_log 2>&1 	
else
	printf "Usage:\nscript <List/list>: Display all available wifi and then exit\nscript <Victor ID> <SSID> <password>\n"
		exit
fi

expect -f connect_to_victor.sh $robot_id $SSID $password 2>&1 | tee -a $robot_id_log


