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
if [ "C" = "$1" ]; then
    robotname="VICTOR_ed3dd857"
    robotip="192.168.40.204"
elif [ "F" = "$1" ]; then
    robotname="VICTOR_1dcae602"
    robotip="192.168.40.200"
elif [ "H" = "$1" ]; then
    robotname="VICTOR_162b1e51"
    robotip="192.168.40.201"
elif [ "J" = "$1" ]; then
    robotname="VICTOR_304aefeb"
    robotip="192.168.40.202"
elif [ "K" = "$1" ]; then
    robotname="VICTOR_2ecbdf7d"
    robotip="192.168.40.203"
fi


if [ -n "$robotname" ] && [ -n "$robotip" ]; then
    expect -f vic_join_robits.sh $robotname $robotip
else
    echo "No config found for robot $1"
fi
