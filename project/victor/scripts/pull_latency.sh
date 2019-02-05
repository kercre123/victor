#!/usr/bin/env bash

# Get the directory of this script
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

source ${SCRIPT_PATH}/host_robot_ip_override.sh

robot_set_host

copy_destination=~/Downloads/latency
mkdir -p $copy_destination

robot_cp_from /data/data/com.anki.victor/cache/anim_latency.json $copy_destination
robot_cp_from /data/data/com.anki.victor/cache/engine_latency.json $copy_destination

open $copy_destination
