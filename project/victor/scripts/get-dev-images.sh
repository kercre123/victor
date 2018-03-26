#!/bin/bash
GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
source ${GIT_PROJ_ROOT}/project/victor/scripts/victor_env.sh

robot_set_host
robot_cp_from -r /data/data/com.anki.victor/cache/camera/images/$1 .
