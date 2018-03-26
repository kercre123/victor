#!/usr/bin/env bash

GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
source ${GIT_PROJ_ROOT}/project/victor/scripts/victor_env.sh

robot_set_host

robot_sh rm -rf /data/data/com.anki.victor
robot_sh rm -rf /anki
