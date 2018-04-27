#!/bin/bash

set -e
set -u


# Get the directory of this script
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=$(basename ${0})
number_rg='^[0-9]+$'
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

result=$(robot_sh netstat -l | awk {'print $1 $4'})
lines=$(echo $result | tr "\n" "\n")
index=0
for line in $lines
do
    if [[ $line = *":"* ]]; then
        proto="${line:0:3}" 
        port=${line##*:}
        if [[ $port =~ $number_rg ]] ; then
           ports[$index]="$proto:$port"
           index=$(( index+1 ))
        fi
    fi
done
echo ${ports[*]}
