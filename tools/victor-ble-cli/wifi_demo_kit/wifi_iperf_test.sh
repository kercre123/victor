#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
IPERF_MAC="$DIR/iperf3-mac"
IPERF_ANDROID="iperf3-android"
IPERF_ANDROID_LOC="$DIR/$IPERF_ANDROID"

ADB=`which adb`
if [ -z $ADB ];then
  echo adb not found
  exit 1
fi

function run_iperf_command {
  IPERF_ANDROID_COMMAND="cd /storage && ./$IPERF_ANDROID -p $IPERF_PORT -c $IP_ADDRESS $*"
  adb shell $IPERF_ANDROID_COMMAND
  echo
  echo
}

adb push $IPERF_ANDROID_LOC /storage/
killall iperf

IPERF_PORT=5201
if [ -z $1 ];then
  IP_ADDRESS="$(ipconfig getifaddr en0)"
  $IPERF_MAC -s -p $IPERF_PORT &
else
  IP_ADDRESS=$1
fi
echo "Running against: $IP_ADDRESS:$IPERF_PORT"


echo "Basic iperf test"
run_iperf_command  -V -t 5 -T "Basic"
echo "Omitting the first 2 seconds"
run_iperf_command  -i .3 -O 2 -t 5 -T "Omit"
echo "IPv4 Only"
run_iperf_command  -4 -t 5 -T "IPv4"
echo "IPv6 Only"
run_iperf_command  -6 -t 5 -T "IPv6"
echo "Parallel streams"
run_iperf_command  -P 3 -t 5 -T "Parallel"
echo "Laptop to Victor"
run_iperf_command  -P 2 -t 5 -R -T "Reverse"
echo "64k Window"
run_iperf_command  -t 5 -w 64k -T "64k-Window"
echo "5M Bytes"
run_iperf_command  -n 5M -T "5M-Bytes"
echo "1K Packets"
run_iperf_command  -k 1K -T "1K-Packets"
echo "Burst"
run_iperf_command  -b1G/100 -T "Burst"
echo "1000 maximum segment size"
run_iperf_command  -M 1000 -V -T "1000-MSS"

killall iperf
