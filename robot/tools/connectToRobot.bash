#!/usr/bin/env bash

SSID=Cozmo_$1

networksetup -setairportpower en0 on
if test -z `system_profiler SPAirPortDataType | grep $SSID`; then
  echo "Scanning for $SSID"
  networksetup -setairportpower en0 off
  sleep 5
  networksetup -setairportpower en0 on
  sleep 2
  while test -z `system_profiler SPAirPortDataType | grep $SSID`; do
    echo "Scanning for $SSID"
    sleep 1
  done
fi

echo "Connecting to $SSID"
if networksetup -setairportnetwork en0 $SSID QQQQQQQQQQQQ; then
    echo "Waiting for IP address"
    ping -o -q 172.31.1.1
    echo "Connected"
else
    echo "Couldn't connect to network"
    exit 1
fi
