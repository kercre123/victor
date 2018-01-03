#!/bin/bash
if [ -z $1 ];then
  echo "Please input robot name"
  read ROBOT_NAME
else
  ROBOT_NAME=$1
fi

if [ -z $2 ];then
  echo "Please input SSID name"
  read SSID_NAME
else
  SSID_NAME=$2
fi

if [ -z $3 ];then
  echo "Please input SSID password"
  read SSID_PASSWORD
else
  SSID_PASSWORD=$3
fi


function write_log {
  while read -r line
  do
    printf '[%s] %s\n' "$(date '+%Y-%m-%d-%H.%M.%S')" "$line" | tee -a $1
  done
}

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
LOG_DIR="$DIR/logs_$(date '+%Y-%m-%d-%H.%M.%S')"
mkdir $LOG_DIR

echo "Testing robot '$ROBOT_NAME' against SSID '$SSID_NAME' with password '$SSID_PASSWORD'"

while [ true ]
do
  echo "Please input room name for logging file name (ctrl+c to quit)"
  read ROOM_LOC_FULL
  ROOM_LOC=${ROOM_LOC_FULL//[^[:alnum:]]/}
  mkdir $LOG_DIR/$ROOM_LOC
  $DIR/gather_wifi_info.sh | write_log $LOG_DIR/$ROOM_LOC/wifi_list.log
  $DIR/connect_to_victor.sh $ROBOT_NAME $SSID_NAME $SSID_PASSWORD 2>&1 | write_log $LOG_DIR/$ROOM_LOC/connect.log
  if [ $? = 0 ];then
    $DIR/wifi_iperf_test.sh 2>&1 | write_log $LOG_DIR/$ROOM_LOC/performance_local.log
    ANKI_SERVER=34.216.156.32
    $DIR/wifi_iperf_test.sh $ANKI_SERVER 2>&1 | write_log $LOG_DIR/$ROOM_LOC/performance_cloud.log
  fi
done
