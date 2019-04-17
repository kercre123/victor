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
: ${IGNORE_COMPATIBILITY_MISMATCH:=0}
: ${IGNORE_VERSION_MISMATCH:=0}
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
  echo "  -i                      ignore compatibility mismatch. Do not use this. Update your robot/branch!"
  echo "  -b                      ignore version mismatch.  Do not use this.  Update your robot/branch!"
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

while getopts "hvrfibc:s:" opt; do
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
    i)
      IGNORE_COMPATIBILITY_MISMATCH=1
      ;;
    b)
      IGNORE_VERSION_MISMATCH=1
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


# echo 0  if versions are equal
# echo -1 if $1 < $2
# echo 1  if $1 > $2
function compare_victor_compat_version() {
    local A_VER=$1
    local B_VER=$2
    if [[ ${A_VER} -gt ${B_VER} ]]; then
        echo 1 && return 0
    elif [[ ${A_VER} -lt ${B_VER} ]]; then
        echo -1 && return 0
    fi
    echo 0
}

# echo 0  if versions are equal
# echo -1 if $1 < $2
# echo 1  if $1 > $2
function compare_anki_version() {
  local A_VER=( ${1//./ })
  local B_VER=( ${2//./ })

  if [ ${#A_VER[@]} -gt ${#B_VER[@]} ]; then
    echo 1 && return 0
  elif [ ${#A_VER[@]} -lt ${#B_VER[@]} ]; then
    echo -1 && return 0
  fi

  # Only compare version numbers that are not build-numbers
  # Either X.Y or X.Y.Z
  local MAX_IDX=$((${#A_VER[@]} - 2))    # subtract 1 for build-number, 1 for zero-index

  for i in $(seq 0 ${MAX_IDX}); do
    if [ ${A_VER[i]} -lt ${B_VER[i]} ]; then
      echo -1 && return 0
    elif [ ${A_VER[i]} -gt ${B_VER[i]} ]; then
      echo 1 && return 0
    fi
  done

  echo 0
}

# run version check
CUR_OS_VERSION=$(robot_sh "cat /etc/os-version")
DEPLOY_VERSION=$(cat ${STAGING_DIR}/anki/etc/version)

VER_CMP=$(compare_anki_version $DEPLOY_VERSION $CUR_OS_VERSION)

if [ ${VER_CMP} -eq 1 ]; then
  # deploy > os :
  echo "Target deploy version (${DEPLOY_VERSION}) is newer than Robot OS version (${CUR_OS_VERSION})."
elif [ ${VER_CMP} -eq -1 ]; then
  # deploy < os
  echo "Target deploy version (${DEPLOY_VERSION}) is older than Robot OS version (${CUR_OS_VERSION})."
fi

if [ ${VER_CMP} -ne 0 ]; then
  if [ $IGNORE_VERSION_MISMATCH -eq 1 ]; then
      printf '%s%s%s%s\n' \
             "$(tput setaf 9)" \
             "$(tput blink)" \
             "Ignoring OS version mismatch.  This is probably a really bad idea!!!" \
             "$(tput sgr0)"
  else
    echo "OS version mismatch. Update your robot or your branch!"
    echo "If you are certain you want to deploy anyway, use the -b option."
    echo "When things don't work, please update your robot and/or your branch and try again!"
    cleanup
    exit 1
  fi
fi

OS_COMPAT_VERSION=$(robot_sh "cat /etc/victor-compat-version || echo 0")
DEPLOY_VERSION=$(cat ${STAGING_DIR}/anki/etc/victor-compat-version)

VER_CMP=$(compare_victor_compat_version $DEPLOY_VERSION $OS_COMPAT_VERSION)

if [[ ${VER_CMP} -gt 0 ]]; then
    echo -e "Target deploy compatibility version (${DEPLOY_VERSION}) is newer than robot version (${OS_COMPAT_VERSION}).\nYou need to upgrade the OS version on your robot.  Try ./project/victor/scripts/robot_sh.sh update-os"
elif [[ ${VER_CMP} -lt 0 ]]; then
    echo -e "Target deploy compatibility version (${DEPLOY_VERSION}) is older than robot version (${OS_COMPAT_VERSION}).\nYou need to rebase your current branch to pick up required changes for compatibility with the robot."
fi

if [[ ${VER_CMP} -ne 0 ]]; then
    if [[ $IGNORE_COMPATIBILITY_MISMATCH -eq 1 ]]; then
        printf '%s%s%s%s\n' \
               "$(tput setaf 9)" \
               "$(tput blink)" \
               "Ignoring compatibility version mismatch.  This is probably a really bad idea!!!" \
               "$(tput sgr0)"
    else
        echo "Compatibility version mismatch.  Update your robot or your branch!"
        echo "If you are certain that you want to deplay anyway, use the -i option."
        echo "When things don't work, please update your robot and/or your branch and try again!"
        cleanup
        exit 1
    fi
fi

set +e
( # TRY deploy
logv "start deploy"

set -e

#
# Stop any victor services. If services are allowed to run during 
# deployment, exe and shared library files can't be replaced.
#
logv "stop victor services"
robot_sh "/bin/systemctl stop victor.target"

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

logv "starting rsync daemon"
robot_sh "/bin/systemctl is-active rsyncd.service > /dev/null 2>&1\
          || /bin/systemctl start rsyncd.service && sleep 0.5"

pushd ${STAGING_DIR} > /dev/null 2>&1

#
# Use --inplace to avoid consuming temp space & minimize number of writes
# Use --delete to purge files that are no longer present in build tree
#
RSYNC_ARGS="-rlptD -zvP --chmod=ug+rwx --chown=:2901 --inplace --delete"
if [ $FORCE_DEPLOY -eq 1 ]; then
  # Ignore times, delete before transfer, and force deletion of directories
  RSYNC_ARGS="-rlptD -IzvP --chmod=ug+rwx --chown=:2901 --inplace --delete --delete-before --force"
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
