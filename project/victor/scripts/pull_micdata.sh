#!/usr/bin/env bash

# Copies the micdata found on the robot device into the user's Downloads folder,
# with a folder name based on the current time

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

micdata_src=/data/data/com.anki.victor/cache/micdata

datetime=$(date '+%m%d-%k%M%S');
copy_destination=~/Downloads/micdata$datetime
mkdir -p $copy_destination

robot_cp_from -r $micdata_src $copy_destination

open $copy_destination
