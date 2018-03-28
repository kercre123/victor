#!/bin/bash

set -e
set -u

# Get the directory of this script                                                                    
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
GIT=`which git`
if [ -z $GIT ]
then
    echo git not found
    exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

source ${SCRIPT_PATH}/victor_env.sh

robot_set_host

robot_sh logcat mm-camera:S mm-camera-intf:S mm-camera-eztune:S mm-camera-sensor:S mm-camera-img:S cnss-daemon:S cozmoengined:S ServiceManager:S chatty:S
