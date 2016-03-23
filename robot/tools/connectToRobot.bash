#!/usr/bin/env bash

networksetup -setairportpower en0 on

while test -z `system_profiler SPAirPortDataType | grep $1`; do
  echo "Scanning for $1"
  sleep 1
done

echo "Connecting to $1"
networksetup -setairportnetwork en0 $1 2manysecrets

echo "Waiting for IP address"
ping -o -q 172.31.1.1

echo "Connected"
