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
: ${TOPLEVEL:=`$GIT rev-parse --show-toplevel`}

source ${SCRIPT_PATH}/victor_env.sh

# Settings can be overridden through environment
: ${VERBOSE:=0}
: ${FORCE_RSYNC_BIN:=0}
: ${FORCE_DEPLOY:=0}
: ${ANKI_BUILD_TYPE:="Debug"}
: ${INSTALL_ROOT:="/anki"}
: ${DEVTOOLS_INSTALL_ROOT:="/anki-devtools"}
: ${DEVICE_RSYNC_BIN_DIR:="${DEVTOOLS_INSTALL_ROOT}/bin"}
: ${DEVICE_RSYNC_CONF_DIR:="/run/systemd/system"}

function usage() {
  echo "$SCRIPT_NAME [OPTIONS]"
  echo "options:"
  echo "  -h                      print this message"
  echo "  -v                      print verbose output"
  echo "  -r                      force-install rsync binary on robot"
  echo "  -f                      force rsync to (re)deploy all files"
  echo "  -c CONFIGURATION        build configuration {Debug,Release}"
  echo "  -s ANKI_ROBOT_HOST      hostname or ip address of robot"
  echo ""
  echo "environment variables:"
  echo '  $ANKI_ROBOT_HOST        hostname or ip address of robot'
  echo '  $ANKI_BUILD_TYPE        build configuration {Debug,Release}'
  echo '  $INSTALL_ROOT           root dir of installed files on target'
  echo '  $STAGING_DIR            directory that holds staged artifacts before deploy to robot'
}

function logv() {
  if [ $VERBOSE -eq 1 ]; then
    echo -n "[$SCRIPT_NAME] "
    echo $*;
  fi
}

while getopts "hvrfc:s:" opt; do
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
    f)
      FORCE_DEPLOY=1
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

: ${PLATFORM_NAME:="vicos"}
: ${LIB_INSTALL_PATH:="${INSTALL_ROOT}/lib"}
: ${BIN_INSTALL_PATH:="${INSTALL_ROOT}/bin"}
: ${RSYNC_BIN_DIR="${TOPLEVEL}/tools/rsync"}
: ${STAGING_DIR:="${TOPLEVEL}/_build/staging/${ANKI_BUILD_TYPE}"}

# Remount rootfs read-write if necessary
MOUNT_STATE=$(\
    robot_sh "grep ' / ext4.*\sro[\s,]' /proc/mounts > /dev/null 2>&1 && echo ro || echo rw"\
)
[[ "$MOUNT_STATE" == "ro" ]] && logv "remount rw /" && robot_sh "/bin/mount -o remount,rw /"

function cleanup() {
  # Remount rootfs read-only if it was previously mounted read-only before deployment
  # This function used to be this single line:
  # [[ "$MOUNT_STATE" == "ro" ]] && logv "remount ro /" && robot_sh "/bin/mount -o remount,ro /"
  # but we don't want this function to return an exit status of 1 when $MOUNT_STATE != "ro"
  if [ "$MOUNT_STATE" == "ro" ]; then
    logv "remount ro /"
    robot_sh "/bin/mount -o remount,ro /"
  fi
}

# trap ctrl-c and call ctrl_c()
trap cleanup INT

set +e
( # TRY deploy
logv "start deploy"

set -e

logv "create target dirs"
robot_sh mkdir -p "${INSTALL_ROOT}"
robot_sh mkdir -p "${INSTALL_ROOT}/etc"
robot_sh mkdir -p "${LIB_INSTALL_PATH}"
robot_sh mkdir -p "${BIN_INSTALL_PATH}"
robot_sh mkdir -p "${DEVICE_RSYNC_BIN_DIR}"

# install rsync binary and config if needed
logv "install rsync if necessary"
set +e
robot_sh [ -f "$DEVICE_RSYNC_BIN_DIR/rsync.bin" ]
if [ $? -ne 0 ] || [ $FORCE_RSYNC_BIN -eq 1 ]; then
  echo "loading rsync to device"
  robot_cp ${RSYNC_BIN_DIR}/rsync.bin ${DEVICE_RSYNC_BIN_DIR}/rsync.bin
fi

robot_sh [ -f "$DEVICE_RSYNC_CONF_DIR/rsyncd.conf" ]
if [ $? -ne 0 ] || [ $FORCE_RSYNC_BIN -eq 1 ]; then
  echo "loading rsync config to device"
  robot_cp ${RSYNC_BIN_DIR}/rsyncd.conf ${DEVICE_RSYNC_CONF_DIR}/rsyncd.conf
fi

robot_sh [ -f "$DEVICE_RSYNC_CONF_DIR/rsyncd.service" ]
if [ $? -ne 0 ] || [ $FORCE_RSYNC_BIN -eq 1 ]; then
  echo "loading rsyncd.service to device"
  robot_cp ${RSYNC_BIN_DIR}/rsyncd.service ${DEVICE_RSYNC_CONF_DIR}/rsyncd.service
  robot_sh "/bin/systemctl daemon-reload"
fi
set -e

#
# Stop any victor services. If services are allowed to run during deployment, exe and shared library 
# files can't be released.  This may tie up enough disk space to prevent deployment of replacement files.
# 
logv "stop victor services"
robot_sh "/bin/systemctl stop victor.target"

logv "starting rsync daemon"
robot_sh "/bin/systemctl is-active rsyncd.service > /dev/null 2>&1\
          || /bin/systemctl start rsyncd.service"

pushd ${STAGING_DIR} > /dev/null 2>&1

#
# Use --inplace to avoid consuming temp space & minimize number of writes
# Use --delete to purge files that are no longer present in build tree
#
RSYNC_ARGS="-rlptD -uzvP --inplace --delete"
if [ $FORCE_DEPLOY -eq 1 ]; then
  # Ignore times, delete before transfer, and force deletion of directories 
  RSYNC_ARGS="-rlptD -IzvP --inplace --delete --delete-before --force"
fi

logv "rsync"
set +e
rsync $RSYNC_ARGS \
  ./anki/ rsync://${ANKI_ROBOT_HOST}:1873/anki_root/
RSYNC_RESULT=$?
set -e

popd > /dev/null 2>&1

logv "stop rsync daemon"
robot_sh "/bin/systemctl stop rsyncd"

logv "finish deploy"

exit $RSYNC_RESULT
) # End TRY deploy

DEPLOY_RESULT=$?
set -e

if [ $DEPLOY_RESULT -eq 0 ]; then
  logv "deploy succeeded"
else
  logv "deploy FAILED"
fi

cleanup

exit $DEPLOY_RESULT
