#!/usr/bin/env bash
#
# project/victor/scripts/robot_sh.sh
#
# Helper script to run shell commands on robot
#
set -eu

# Get path to this script
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))

source ${SCRIPT_PATH}/victor_env.sh

source ${SCRIPT_PATH}/host_robot_ip_override.sh

robot_set_host

robot_sh "$@"

