#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

robotname=""
robotip=""

if [ $# -eq 0 ]; then
    echo "No robot ID supplied"
    exit 1
fi


# Begin robot configs
if [ "F" = "$1" ]; then
    robotname="VICTOR_1dcae602"
    robotip="192.168.40.200"
fi


if [ -n "$robotname" ] && [ -n "$robotip" ]; then
    expect -f vic_join_robits.sh $robotname $robotip
else
    echo "No config found for robot $1"
fi
