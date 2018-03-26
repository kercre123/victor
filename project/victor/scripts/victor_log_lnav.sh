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

robot_sh logcat | lnav -c '\'':filter-out mm-camera'\'' -c '\'':filter-out cnss-daemon'\'' -c '\'':filter-out ServiceManager'\'' -c '\'':filter-out chatty'\'''
