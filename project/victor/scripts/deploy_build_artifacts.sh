#!/bin/bash

set -e
set -u

# Get directory of this script, then normalize to absolute path
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_PATH=$(cd ${SCRIPT_PATH}; echo ${PWD})

SCRIPT_NAME=$(basename ${0})
TOPLEVEL=$SCRIPT_PATH

source ${SCRIPT_PATH}/victor_env.sh

# Settings can be overridden through environment
: ${VERBOSE:=0}
: ${INSTALL_ROOT:="/anki"}

function usage() {
  echo "$SCRIPT_NAME [OPTIONS]"
  echo "  -h                      print this message"
  echo "  -v                      print verbose output"
  echo "  -s ANKI_ROBOT_HOST      hostname or ip address of robot"
  echo ""
  echo "environment variables:"
  echo '  $ANKI_ROBOT_HOST        hostname or ip address of robot'
  echo '  $BUILD_ROOT             root dir of build artifacts containing {bin,lib,etc,data} dirs'
  echo '  $INSTALL_ROOT           root dir of installed files on target'
}

while getopts "hv:s:" opt; do
  case $opt in
    h)
      usage && exit 0
      ;;
    v)
      VERBOSE=1
      ;;
    s)
      ANKI_ROBOT_HOST="${OPTARG}"
      ;;
    *)
      usage && exit 1
      ;;
  esac
done

if [ -z "${ANKI_ROBOT_HOST+x}" ]; then
  echo "ERROR: unspecified robot target. Pass the '-s' flag with robot IP, or set ANKI_ROBOT_HOST"
  usage
  exit 1
fi

: ${DEVICE_RSYNC_BIN_DIR:="/tmp"}
: ${DEVICE_RSYNC_CONF_DIR:="/data/rsync"}

# increment the following value if the contents of rsyncd.conf change
RSYNCD_CONF_VERSION=2

: ${BUILD_ROOT:="${TOPLEVEL}"}
: ${LIB_INSTALL_PATH:="${INSTALL_ROOT}/lib"}
: ${BIN_INSTALL_PATH:="${INSTALL_ROOT}/bin"}
: ${RSYNC_BIN_DIR="${TOPLEVEL}/rsync"}

echo "Stopping processes"
robot_sh "/bin/systemctl stop victor.target"

robot_sh mkdir -p "${INSTALL_ROOT}"
robot_sh mkdir -p "${INSTALL_ROOT}/etc"
robot_sh mkdir -p "${LIB_INSTALL_PATH}"
robot_sh mkdir -p "${BIN_INSTALL_PATH}"

# install rsync binary and config if needed
set +e
robot_sh [ -f "$DEVICE_RSYNC_BIN_DIR/rsync.bin" ]
if [ $? -ne 0 ]; then
  echo "loading rsync to device"
  robot_cp ${RSYNC_BIN_DIR}/rsync.bin ${DEVICE_RSYNC_BIN_DIR}/rsync.bin
fi
set -e

RSYNCD_CONF="rsyncd-v${RSYNCD_CONF_VERSION}.conf"

robot_sh "[ -f "$DEVICE_RSYNC_CONF_DIR/$RSYNCD_CONF" ]"
if [ $? -ne 0 ]; then
  echo "loading rsync config to device"
  robot_cp ${RSYNC_BIN_DIR}/rsyncd.conf ${DEVICE_RSYNC_CONF_DIR}/$RSYNCD_CONF
fi
set -e

# startup rsync daemon
robot_sh "${DEVICE_RSYNC_BIN_DIR}/rsync.bin --daemon --config=${DEVICE_RSYNC_CONF_DIR}/${RSYNCD_CONF}"

pushd ${BUILD_ROOT} > /dev/null 2>&1

# Since invoking rsync multiple times is expensive.
# build an include file list so that we can run a single rsync command
RSYNC_LIST="${BUILD_ROOT}/rsync.$$.lst"
touch ${RSYNC_LIST}

find lib -type f -name '*.so' >> ${RSYNC_LIST}
find bin -type f -not -name '*.full' >> ${RSYNC_LIST}
find etc >> ${RSYNC_LIST}
find data >> ${RSYNC_LIST}

#
# Use --inplace to avoid consuming temp space & minimize number of writes
# Use --delete to purge files that are no longer present in build tree
#
rsync -rlptD -IzvP --inplace --delete --delete-before --force --files-from=${RSYNC_LIST} \
  --rsync-path=${DEVICE_RSYNC_BIN_DIR}/rsync.bin \
  -e ssh \
  ./ root@${ANKI_ROBOT_HOST}:/anki/

rm -f ${BUILD_ROOT}/rsync.*.lst

popd > /dev/null 2>&1

echo "Restarting processes"
robot_sh systemctl restart anki-robot.target
