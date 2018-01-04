#!/usr/bin/env bash
wifi_count=0

#we need to disconnect in order to get scan results for the AP we connect to
adb shell timeout 30s wpa_cli disconnect 2>&1

adb shell wpa_cli scan 2>&1
sleep 2
wifi_count="$(adb shell wpa_cli scan_result | awk '{count++} END {print count}')"
printf "\n\nWifi count is $wifi_count\n\n"

printf "Detailed list of Wifi\n\n"
adb shell wpa_cli scan_result 2>&1

