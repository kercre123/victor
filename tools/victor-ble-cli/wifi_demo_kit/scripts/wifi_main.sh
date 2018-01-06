#!/bin/bash
ADB=`which adb`
if [ -z $ADB ];then
  echo adb not found
  exit 1
fi

ADB_DEVICES=$(adb devices)
if [ "$ADB_DEVICES" = "List of devices attached" ]; then
  echo "============================================================================================"
  echo "=== No attached Victor found, please make sure Victor is connected and the lights are on ==="
  echo "============================================================================================"
  exit 1
fi

shopt -s nocasematch
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
LOG_DIR="$DIR/../logs/logs_$(date '+%Y-%m-%d-%H.%M.%S')"
mkdir -p $LOG_DIR

if [ -z $1 ];then
  read -p "SSID/WiFi Network Name: " SSID_NAME
else
  SSID_NAME=$1
fi
printf "SSID/WiFi Network Name: %s\n" "$SSID_NAME" > $LOG_DIR/configuration.log

if [ -z $2 ];then
  read -p "SSID/WiFi Network Password: " SSID_PASSWORD
else
  SSID_PASSWORD=$2
fi
printf "SSID/WiFi Network Password: %s\n" "$SSID_PASSWORD" >> $LOG_DIR/configuration.log

read -p "Current location of the Router: " ROUTER_LOC
printf "Router Location: %s\n" "$ROUTER_LOC" >> $LOG_DIR/configuration.log


function write_log {
  while read -r line
  do
    printf '[%s] %s\n' "$(date '+%Y-%m-%d-%H.%M.%S')" "$line" | tee -a $1
  done
}

RUN_COUNT=1

while [ true ]
do
  echo "=============================================================================================================="
  echo "Testing against SSID '$SSID_NAME' with password '$SSID_PASSWORD'"
  echo "If this isn't the correct SSID or Password, type \"exit\" to quit, and re-run the script with the correct information"
  echo "Please input the current location of Victor, normally a room name (type \"exit\" to quit)"
  echo "=============================================================================================================="
  read -p "Victor Location: " ROOM_LOC_USER
  if [ "$ROOM_LOC_USER" = "exit" ]; then
    exit
  fi
  ROOM_LOC=${ROOM_LOC_USER//[^[:alnum:]]/}
  FOLDER_LOC=$LOG_DIR/$RUN_COUNT-$ROOM_LOC
  ((RUN_COUNT++))
  mkdir $FOLDER_LOC
  $DIR/gather_wifi_info.sh | write_log $FOLDER_LOC/wifi_list.log
  $DIR/connect_to_victor_adb.sh $SSID_NAME $SSID_PASSWORD 2>&1 | write_log $FOLDER_LOC/connect.log
  if [ ${PIPESTATUS[0]} = 0 ];then
    $DIR/wifi_iperf_test.sh 2>&1 | write_log $FOLDER_LOC/performance_local.log
    ANKI_SERVER=34.216.156.32
    $DIR/wifi_iperf_test.sh $ANKI_SERVER 2>&1 | write_log $FOLDER_LOC/performance_cloud.log
  else
    echo "========================================="
    echo "===CONNECTION FAILED, PLEASE TRY AGAIN==="
    echo "========================================="
  fi
done
