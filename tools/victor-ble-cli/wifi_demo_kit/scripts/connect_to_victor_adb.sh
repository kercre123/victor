#!/usr/bin/env bash

SSID=$1
PASSWORD=$2
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

function run_adb_command {
  echo
  echo
  echo $*
  adb shell timeout -t 30 $* 2>&1
}

function run_adb_command_no_output {
  adb shell timeout -t 30 $* 2>&1
}

echo "Starting add_network"
output=$(run_adb_command wpa_cli add_network)
printf "$output\n"
while read -r line
do
  NETWORK_NUM=$line #Looking for the last line which is the network we created
done <<< "$output"

run_adb_command ifconfig wlan0 down
run_adb_command ifconfig wlan0 up

run_adb_command wpa_cli set_network $NETWORK_NUM ssid \\\"$SSID\\\"
run_adb_command_no_output wpa_cli set_network $NETWORK_NUM psk \\\"$PASSWORD\\\"
run_adb_command wpa_cli select_network $NETWORK_NUM
run_adb_command wpa_cli reassociate
run_adb_command wpa_cli status

echo "Starting DHCP"
run_adb_command killall dhcpcd
run_adb_command dhcpcd -w wlan0
DHCP_TIMEOUT=$?

#Some good information about the results
run_adb_command ifconfig
run_adb_command wpa_cli status
run_adb_command iwconfig
run_adb_command iwlist wlan0 channel
run_adb_command iwlist wlan0 bitrate
run_adb_command iwlist wlan0 encryption
run_adb_command iwlist wlan0 txpower
run_adb_command iwlist wlan0 peers
run_adb_command iwlist wlan0 wpakeys
run_adb_command ping -c 5 8.8.8.8

exit $DHCP_TIMEOUT
