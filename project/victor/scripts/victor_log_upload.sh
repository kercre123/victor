#!/usr/bin/env bash
#
# project/victor/scripts/victor_log_upload.sh
#
# Helper script to upload log files from robot
#
set -eu

GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
SCRIPTS=${GIT_PROJ_ROOT}/project/victor/scripts
ROBOT_SH=${SCRIPTS}/robot_sh.sh

#
# These values are used to establish blobstore credentials.
# These values should not be used in production environment
# and should not be stored on the robot.
#
# Application key is fetched at build time via top-level DEPS.
# It can also be set in environment for use outside of build tree.
#
if [ -z ${APP_URL+x} ]; then
  APP_URL="https://blobstore-dev.api.anki.com/1/qavictor/blobs"
fi

if [ -z ${APP_KEY+x} ]; then
  keyfile="${GIT_PROJ_ROOT}/EXTERNALS/victor-blobstore-qalogs-key"
  APP_KEY="`/bin/cat ${keyfile}`"
  if [ -z ${APP_KEY+x} ]; then
    echo "Can't run without application key"
    exit 1
  fi
fi

if [ -z ${APP_USER+x} ]; then
  APP_USER="${USER}"
fi

${ROBOT_SH} "/anki/bin/vic-logmgr-upload ${APP_URL} ${APP_KEY} ${APP_USER}"
