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

source ${SCRIPT_PATH}/android_env.sh
export -f adb_shell

# Settings can be overridden through environment
: ${VERBOSE:=0}
: ${ANKI_BUILD_TYPE:="Debug"}
: ${INSTALL_ROOT:="/anki"}

function usage() {
  echo "$SCRIPT_NAME [OPTIONS]"
  echo "  -h                      print this message"
  echo "  -v                      print verbose output"
  echo "  -c [CONFIGURATION]      build configuration {Debug,Release}"
}

while getopts "hvc:" opt; do
  case $opt in
    h)
      usage && exit 0
      ;;
    v)
      VERBOSE=1
      ;;
    c)
      ANKI_BUILD_TYPE="${OPTARG}"
      ;;
    *)
      usage && exit 1
      ;;
  esac
done

# Settings can be overridden through environment
: ${VERBOSE:=0}
: ${ANKI_BUILD_TYPE:="Debug"}
: ${INSTALL_ROOT:="/anki"}

: ${DEVICE_RSYNC_BIN_DIR:="/tmp"}
: ${DEVICE_RSYNC_CONF_DIR:="/data/rsync"}

# increment the following value if the contents of rsyncd.conf change
RSYNCD_CONF_VERSION=2


function usage() {
  echo "$SCRIPT_NAME [OPTIONS]"
  echo "  -h                      print this message"
  echo "  -v                      print verbose output"
  echo "  -c [CONFIGURATION]      build configuration {Debug,Release}"
}

while getopts "hvc:" opt; do
  case $opt in
    h)
      usage && exit 0
      ;;
    v)
      VERBOSE=1
      ;;
    c)
      ANKI_BUILD_TYPE="${OPTARG}"
      ;;
    *)
      usage && exit 1
      ;;
  esac
done

# echo "VERBOSE: ${VERBOSE}"
# echo "ANKI_BUILD_TYPE: ${ANKI_BUILD_TYPE}"
echo "INSTALL_ROOT: ${INSTALL_ROOT}"

: ${PLATFORM_NAME:="android"}
: ${BUILD_ROOT:="${TOPLEVEL}/_build/${PLATFORM_NAME}/${ANKI_BUILD_TYPE}"}
: ${LIB_INSTALL_PATH:="${INSTALL_ROOT}/lib"}
: ${BIN_INSTALL_PATH:="${INSTALL_ROOT}/bin"}
: ${RSYNC_BIN_DIR="${TOPLEVEL}/tools/rsync"}

$ADB shell mkdir -p "${INSTALL_ROOT}"
$ADB shell mkdir -p "${INSTALL_ROOT}/etc"
$ADB shell mkdir -p "${LIB_INSTALL_PATH}"
$ADB shell mkdir -p "${BIN_INSTALL_PATH}"

# get device IP Address
DEVICE_IP_ADDRESS=`$ADB shell ip addr show wlan0 | grep "inet\s" | awk '{print $2}' | awk -F'/' '{print $1}'`
if [ -z $DEVICE_IP_ADDRESS ]; then
  DEVICE_IP_ADDRESS=`$ADB shell ip addr show lo | grep "inet\s" | awk '{print $2}' | awk -F'/' '{print $1}'`
  if [ -z $DEVICE_IP_ADDRESS ]; then
    echo "no valid android device found"
    exit 1
  fi

  DEVICE_IP_ADDRESS="$DEVICE_IP_ADDRESS:6010"
  $ADB forward tcp:6010 tcp:1873
else
  DEVICE_IP_ADDRESS="$DEVICE_IP_ADDRESS:1873"
fi

# install rsync binary and config if needed
set +e
adb_shell "[ -f "$DEVICE_RSYNC_BIN_DIR/rsync.bin" ]"
if [ $? -ne 0 ]; then
  echo "loading rsync to device"
  $ADB push ${RSYNC_BIN_DIR}/rsync.bin ${DEVICE_RSYNC_BIN_DIR}/rsync.bin
fi

RSYNCD_CONF="rsyncd-v${RSYNCD_CONF_VERSION}.conf"

adb_shell "[ -f "$DEVICE_RSYNC_CONF_DIR/$RSYNCD_CONF" ]"
if [ $? -ne 0 ]; then
  echo "loading rsync config to device"
  $ADB push ${RSYNC_BIN_DIR}/rsyncd.conf ${DEVICE_RSYNC_CONF_DIR}/$RSYNCD_CONF
fi
set -e

# startup rsync daemon
$ADB shell "${DEVICE_RSYNC_BIN_DIR}/rsync.bin --daemon --config=${DEVICE_RSYNC_CONF_DIR}/${RSYNCD_CONF}"

#
# Stop any victor services. If services are allowed to run during deployment, exe and shared library 
# files can't be released.  This may tie up enough disk space to prevent deployment of replacement files.
# 
$ADB shell "/bin/systemctl stop victor.target"

pushd ${BUILD_ROOT} > /dev/null 2>&1

# Since invoking rsync multiple times is expensive.
# build an include file list so that we can run a single rsync command
RSYNC_LIST="${BUILD_ROOT}/rsync.$$.lst"
touch ${RSYNC_LIST}

if [ -d lib ]; then
  find lib -type f -name '*.so' -or -name '*.so.1' >> ${RSYNC_LIST}
fi

if [ -d bin ]; then
  find bin -type f -not -name '*.full' >> ${RSYNC_LIST}
fi

if [ -d etc ]; then
  find etc >> ${RSYNC_LIST}
fi

if [ -d data ]; then
  find data >> ${RSYNC_LIST}
fi

#
# Use --inplace to avoid consuming temp space & minimize number of writes
# Use --delete to purge files that are no longer present in build tree
#
rsync -rlptD -uzvP --inplace --delete --files-from=${RSYNC_LIST} ./ rsync://${DEVICE_IP_ADDRESS}/anki_root/

rm -f ${BUILD_ROOT}/rsync.*.lst

popd > /dev/null 2>&1
