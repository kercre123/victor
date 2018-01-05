#!/bin/bash
shopt -s nocasematch
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
LOG_DIR="$DIR/logs_$(date '+%Y-%m-%d-%H.%M.%S')"
mkdir $LOG_DIR

if [ -z $1 ];then
  read -p "SSID Name: " SSID_NAME
else
  SSID_NAME=$1
fi
printf "SSID Name: %s\n" "$SSID_NAME" > $LOG_DIR/configuration.log

if [ -z $2 ];then
  read -p "SSID Password: " SSID_PASSWORD
else
  SSID_PASSWORD=$2
fi
printf "SSID Password: %s\n" "$SSID_PASSWORD" >> $LOG_DIR/configuration.log

read -p "Router Locaion: " ROUTER_LOC
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
  echo "If this isn't the correct SSID or Password, enter exit to exit, and re-run the script with the correct information"
  echo "Please input the room location (enter exit to quit)"
  echo "=============================================================================================================="
  read -p "Roomname: " ROOM_LOC_USER
  if [ $ROOM_LOC_USER = "exit" ]; then
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
