#! /bin/bash

PIN=$(expr $RANDOM % 9 + 1)
for i in {1..5}
do
  R=$(expr $RANDOM + $RANDOM)
  PIN+=$(expr $R % 10)
done

ssh root@$1 "mount -o remount,rw /factory && \
  echo $PIN > /factory/ble_pairing_pin && \
  mount -o remount,ro /factory"

if [ $? -eq 0 ]; then
  echo $PIN
  exit 0
else
  echo "Error: Could not connect over SSH."
  exit 1
fi
