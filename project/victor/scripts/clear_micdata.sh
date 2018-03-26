#!/usr/bin/env bash

# Copies the micdata found on the robot device into the user's Downloads folder,
# with a folder name based on the current time
GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
source ${GIT_PROJ_ROOT}/project/victor/scripts/victor_env.sh

robot_set_host

micdata_src=/data/data/com.anki.victor/cache/micdata

robot_sh rm -rf $micdata_src
