#!/usr/bin/env bash

# Deletes the messages (recorded via the "leave a message" behavior) found on the robot device

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

GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
source ${GIT_PROJ_ROOT}/project/victor/scripts/victor_env.sh

source ${GIT_PROJ_ROOT}/project/victor/scripts/host_robot_ip_override.sh

robot_set_host

messages_src=/data/data/com.anki.victor/persistent/messages

robot_sh rm -rf $messages_src
