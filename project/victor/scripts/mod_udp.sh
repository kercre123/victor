#!/bin/bash

set -e
set -u

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

$ADB shell sysctl -w net.core.rmem_default=31457280
$ADB shell sysctl -w net.core.rmem_max=12582912
$ADB shell sysctl -w net.core.wmem_default=31457280
$ADB shell sysctl -w net.core.wmem_max=12582912
$ADB shell sysctl -w net.core.optmem_max=25165824
$ADB shell sysctl -w net.ipv4.udp_rmem_min=16384
$ADB shell sysctl -w net.ipv4.udp_wmem_min=16384
$ADB shell sysctl -w net.ipv4.udp_mem="65536 131072 262144"
