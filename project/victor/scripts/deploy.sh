#!/bin/bash

set -e
set -u

# Go to directory of this script
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=$(basename ${0})
GIT=`which git`
if [ -z $GIT ]
then
    echo git not found
    exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

source ${SCRIPT_PATH}/victor_env.sh

# Settings can be overridden through environment
: ${VERBOSE:=0}
: ${FORCE_RSYNC_BIN:=0}
: ${ANKI_BUILD_TYPE:="Debug"}
: ${INSTALL_ROOT:="/anki"}
: ${DEVICE_RSYNC_BIN_DIR:="/usr/bin"}

function usage() {
  echo "$SCRIPT_NAME [OPTIONS]"
  echo "options:"
  echo "  -h                      print this message"
  echo "  -v                      print verbose output"
  echo "  -r                      force-install rsync binary on robot"
  echo "  -c CONFIGURATION        build configuration {Debug,Release}"
  echo "  -s ANKI_ROBOT_HOST      hostname or ip address of robot"
  echo ""
  echo "environment variables:"
  echo '  $ANKI_ROBOT_HOST        hostname or ip address of robot'
  echo '  $ANKI_BUILD_TYPE        build configuration {Debug,Release}'
  echo '  $BUILD_ROOT             root dir of build artifacts containing {bin,lib,etc,data} dirs'
  echo '  $INSTALL_ROOT           root dir of installed files on target'
  echo '  $STAGING_DIR            directory to hold staged artifacts before deploy to robot'
}

while getopts "hvrc:s:" opt; do
  case $opt in
    h)
      usage && exit 0
      ;;
    v)
      VERBOSE=1
      ;;
    r)
      FORCE_RSYNC_BIN=1
      ;;
    c)
      ANKI_BUILD_TYPE="${OPTARG}"
      ;;
    s)
      ANKI_ROBOT_HOST="${OPTARG}"
      ;;
    *)
      usage && exit 1
      ;;
  esac
done

robot_set_host

if [ -z "${ANKI_ROBOT_HOST+x}" ]; then
  echo "ERROR: unspecified robot target. Pass the '-s' flag or set ANKI_ROBOT_HOST"
  usage
  exit 1
fi

# echo "ANKI_BUILD_TYPE: ${ANKI_BUILD_TYPE}"
echo "ANKI_ROBOT_HOST: ${ANKI_ROBOT_HOST}"
echo "   INSTALL_ROOT: ${INSTALL_ROOT}"

: ${PLATFORM_NAME:="android"}
: ${BUILD_ROOT:="${TOPLEVEL}/_build/${PLATFORM_NAME}/${ANKI_BUILD_TYPE}"}
: ${LIB_INSTALL_PATH:="${INSTALL_ROOT}/lib"}
: ${BIN_INSTALL_PATH:="${INSTALL_ROOT}/bin"}
: ${RSYNC_BIN_DIR="${TOPLEVEL}/tools/rsync"}
: ${STAGING_DIR:="${BUILD_ROOT}/staging"}

robot_sh mkdir -p "${INSTALL_ROOT}"
robot_sh mkdir -p "${INSTALL_ROOT}/etc"
robot_sh mkdir -p "${LIB_INSTALL_PATH}"
robot_sh mkdir -p "${BIN_INSTALL_PATH}"

# install rsync binary and config if needed
set +e
robot_sh [ -f "$DEVICE_RSYNC_BIN_DIR/rsync.bin" ]
if [ $? -ne 0 ] || [ $FORCE_RSYNC_BIN -eq 1 ]; then
  echo "loading rsync to device"
  robot_cp ${RSYNC_BIN_DIR}/rsync.bin ${DEVICE_RSYNC_BIN_DIR}/rsync.bin
fi
set -e

#
# Stop any victor services. If services are allowed to run during deployment, exe and shared library 
# files can't be released.  This may tie up enough disk space to prevent deployment of replacement files.
# 
robot_sh "/bin/systemctl stop victor.target"

# install.sh is a script that is run here by deploy.sh, but also independently by
# bitbake when building the Victor OS.
${SCRIPT_PATH}/install.sh ${BUILD_ROOT} ${STAGING_DIR}


pushd ${STAGING_DIR} > /dev/null 2>&1

# Since invoking rsync multiple times is expensive.
# build an include file list so that we can run a single rsync command
rm -rf ${BUILD_ROOT}/rsync.*.lst
RSYNC_LIST="${BUILD_ROOT}/rsync.$$.lst"

find . -type f > ${RSYNC_LIST}

#
# Use --inplace to avoid consuming temp space & minimize number of writes
# Use --delete to purge files that are no longer present in build tree
#
rsync -rlptD -uzvP \
  --inplace \
  --delete \
  --rsync-path=${DEVICE_RSYNC_BIN_DIR}/rsync.bin \
  --files-from=${RSYNC_LIST} \
  -e ssh \
  ./ root@${ANKI_ROBOT_HOST}:/

rm -f "${RSYNC_LIST}"

popd > /dev/null 2>&1
