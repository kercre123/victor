#!/bin/bash

set -e
set -u

# Go to directory of this script
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=$(basename ${0})
GIT=`which git`
if [ -z $GIT ]
then
    echo git not found
    exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

source ${SCRIPT_PATH}/victor_env.sh

robot_set_host

while true; do 
  # Get frequency and temperature
  # (Remove trailing newline from cat output)
  FREQ="$(robot_sh "cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq | xargs echo -n")"
  TEMP="$(robot_sh "cat /sys/devices/virtual/thermal/thermal_zone7/temp")"

  echo "Freq_kHz: ${FREQ}, Temp_C: ${TEMP}"
  sleep 1
done