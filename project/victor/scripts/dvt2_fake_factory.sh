#!/bin/bash

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

function usage() {
  echo "$SCRIPT_NAME [OPTIONS]"
  echo "Creates fake factory related files on robot" 
  echo "options:"
  echo "  -h                      print this message"
  echo "  -s ANKI_ROBOT_HOST      hostname or ip address of robot"
  echo ""
  echo "environment variables:"
  echo '  $ANKI_ROBOT_HOST        hostname or ip address of robot'
}

while getopts "h:s:" opt; do
  case $opt in
    h)
      usage && exit 0
      ;;
    s)
      ANKI_ROBOT_HOST="${OPTARG}"
      ;;
    *)
      usage && exit 1
      ;;
  esac
done

robot_set_host

if [ -z "${ANKI_ROBOT_HOST+x}" ]; then
  echo "ERROR: unspecified robot target. Pass the '-s' flag or set ANKI_ROBOT_HOST"
  usage
  exit 1
fi

echo "ANKI_ROBOT_HOST: ${ANKI_ROBOT_HOST}"

echo "Checking if /factory exists"
robot_sh test -d "/factory"
if [ $? -ne 0 ]; then
  echo "Creating /factory"
  robot_sh mkdir -p "/factory"
fi

REMOUNT_RO=0
function remount() {
  if [ $REMOUNT_RO -eq 0 ]; then
    robot_sh touch "/factory/test"
    if [ $? -ne 0 ]; then
      echo "Remounting /factory rw"
      robot_sh mount -o remount, rw /factory
      REMOUNT_RO=1
    else
      robot_sh rm "/factory/test"
      REMOUNT_RO=2
    fi
  fi
}

echo "Checking for birthcertificate"
robot_sh test -f "/factory/birthcertificate"
if [ $? -ne 0 ]; then
  echo "Creating birthcertificate"
  remount
  robot_sh "dd if=/dev/zero of=/factory/birthcertificate count=1 bs=1024"
fi

echo "Checking for /factory/nvStorage"
robot_sh test -d "/factory/nvStorage"
if [ $? -ne 0 ]; then
  echo "Creating /factory/nvStorage"
  remount
  robot_sh mkdir -p "/factory/nvStorage"
fi

echo "Checking for camera calibration"
robot_sh test -f "/factory/nvStorage/80000001.nvdata"
if [ $? -ne 0 ]; then
  echo "Creating camera calibration"
  remount
  robot_sh mkdir -p "/factory/nvStorage"
  robot_sh touch "/factory/nvStorage/80000001.nvdata"
  # Packed dummy camera calibration
  robot_sh 'printf "\x75\x5c\xb6\x43\xae\x15\xb7\x43\x2f\x50\x9b\x43\x45\xac\x44\x43\x00\x00\x00\x00\x68\x01\x80\x02\x0f\x96\x1c\xbd\x8c\xc4\x97\xbe\xb6\x5b\xed\xba\xee\x96\xf4\x3a\xd4\xab\x38\x3e\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" > /factory/nvStorage/80000001.nvdata'
fi

if [ $REMOUNT_RO -eq 1 ]; then
  robot_sh mount -o remount, ro /factory   
fi

