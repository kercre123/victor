#!/bin/bash

# Tool for clearing shared memory segments that sometimes get orphaned occur when using Webots simulator.
# If you are see red warning when you start the simulator mentioning some problem about shared memory
# this will probably fix it.
#
# NOTE: THIS REMOVES ALL SHARED MEMORY SEGMENTS, SEMAPHORES AND QUEUES!!!
# Make sure you know that you aren't using shared memory for any other active programs.


ME=`whoami`

IPCS_S=`ipcs -s | egrep "[0-9a-f]+ [0-9]+" | grep $ME | cut -f2 -d" "`
IPCS_M=`ipcs -m | egrep "[0-9a-f]+ [0-9]+" | grep $ME | cut -f2 -d" "`
IPCS_Q=`ipcs -q | egrep "[0-9a-f]+ [0-9]+" | grep $ME | cut -f2 -d" "`


for id in $IPCS_M; do
  ipcrm -m $id;
done

for id in $IPCS_S; do
  ipcrm -s $id;
done

for id in $IPCS_Q; do
  ipcrm -q $id;
done

# Kill parent processes if there are any
CPID=`ipcs -p | egrep "[0-9a-f]+ [0-9]+" | grep $ME | awk -F" " '{print $7}'`
LPID=`ipcs -p | egrep "[0-9a-f]+ [0-9]+" | grep $ME | awk -F" " '{print $8}'`

for id in $CPID; do 
  kill -9 $id;
done

for id in $LPID; do
  kill -9 $id;
done

