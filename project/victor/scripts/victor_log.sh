#!/bin/bash

set -e
set -u

# Get the directory of this script
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=$(basename ${0})

function usage() {
  echo "$SCRIPT_NAME [OPTIONS]"
  echo "  -h         print this message and exit"
  echo "  -c         purge logs"
  echo "  -a         show _all_ previous logs (not just most recent)"
}

PURGE_LOGS=0
SHOW_ALL_LOGS=0

while getopts "hca" opt; do
  case $opt in
    h)
      usage
      exit 1
      ;;
    c)
      PURGE_LOGS=1
      ;;
    a)
      SHOW_ALL_LOGS=1
      ;;
    *)
      exit 1
  esac
done

source ${SCRIPT_PATH}/victor_env.sh ""

source ${SCRIPT_PATH}/host_robot_ip_override.sh ""

robot_set_host

LOG_ZIP=/var/log/messages.1.gz

if [ $PURGE_LOGS -eq 1 ]; then
  robot_sh rm -rf $LOG_ZIP
  robot_sh '> /var/log/messages'
  exit $?
fi

if [ $SHOW_ALL_LOGS -eq 1 ]; then
  robot_sh ls $LOG_ZIP 2>/dev/null && robot_sh zcat $LOG_ZIP
fi

robot_sh /bin/tail -F -n +1 /var/log/messages \| /bin/tr '\\x1f' ':'
