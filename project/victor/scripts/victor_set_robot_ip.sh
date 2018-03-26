#!/usr/bin/env bash

GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
IP_FILE=${GIT_PROJ_ROOT}/tools/victor-ble-cli/robot_ip.txt
echo $1 > $IP_FILE

echo "Wrote IP address $1 to $IP_FILE"
