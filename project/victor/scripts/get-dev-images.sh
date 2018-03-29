#!/bin/bash

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

source ${TOPLEVEL}/project/victor/scripts/victor_env.sh

source ${TOPLEVEL}/project/victor/scripts/host_robot_ip_override.sh

robot_set_host
robot_cp_from -r /data/data/com.anki.victor/cache/camera/images/$1 .
