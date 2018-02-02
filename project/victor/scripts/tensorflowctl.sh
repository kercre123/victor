#!/bin/bash

set -e
set -u

: ${INSTALL_ROOT:="/data/data/com.anki.cozmoengine"}

# Go to directory of this script                                                                    
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
GIT=`which git`
if [ -z $GIT ]
then
    echo git not found
    exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

source ${SCRIPT_PATH}/android_env.sh

$ADB shell "${INSTALL_ROOT}/tensorflowctl.sh $*"
