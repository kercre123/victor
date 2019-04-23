#! /bin/bash

function getChar() {
  R=$(expr $RANDOM + $RANDOM)
  R=$(expr $R % 26)
  R=$(expr $R + 65)
  echo $(printf "\\$(printf '%03o' "$R")")
}

function getDigit() {
  R=$(expr $RANDOM + $RANDOM)
  echo $(expr $R % 10)
}

while getopts ":r:" opt; do
  case ${opt} in
    r )
      IP_ADDRESS=$OPTARG
      ;;
    \? )
      echo "Invalid option: $OPTARG" 1>&2
      ;;
    : )
      echo "Invalid option: $OPTARG requires an argument" 1>&2
      ;;
  esac
done

if [ -z "$IP_ADDRESS" ]; then
  IP_ADDRESS=$ANKI_ROBOT_HOST
fi

PIN=$(expr $RANDOM % 9 + 1)
for i in {1..5}
do
  R=$(expr $RANDOM + $RANDOM)
  PIN+=$(expr $R % 10)
done

NAME="Vector Q"
NAME+=$(getDigit)
NAME+=$(getChar)
NAME+=$(getDigit)

ssh root@$IP_ADDRESS "mount -o remount,rw /factory && \
  echo $PIN > /factory/ble_pairing_pin && \
  echo $NAME > /factory/name && \
  mount -o remount,ro /factory"

if [ $? -eq 0 ]; then
  echo -e "$PIN\n$NAME"
  exit 0
else
  echo "Error: Could not connect over SSH."
  exit 1
fi
