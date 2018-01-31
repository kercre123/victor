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
: ${INSTALL_ROOT:="/anki/svc"}

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

: ${BUILD_ROOT:="${TOPLEVEL}/_build/android/${ANKI_BUILD_TYPE}"}
: ${LIB_INSTALL_PATH:="${INSTALL_ROOT}/lib"}
: ${BIN_INSTALL_PATH:="${INSTALL_ROOT}/bin"}

$ADB shell mkdir -p "${INSTALL_ROOT}"
$ADB shell mkdir -p "${INSTALL_ROOT}/etc"
$ADB shell mkdir -p "${LIB_INSTALL_PATH}"
$ADB shell mkdir -p "${BIN_INSTALL_PATH}"

function log_v()
{
    if [ $VERBOSE -eq 1 ]; then
        echo "$1"
    fi
}

function adb_deploy()
{
    SRC="$1"
    DST="$2"
    
    XATTR_KEY="user.com.anki.digest.md5"

    NEEDS_INSTALL=0
    DIGEST=
    XATTR="${BIN_INSTALL_PATH}/axattr"

    FILENAME=$(basename "${SRC}")
    DST_PATH="${DST}/${FILENAME}"

    echo "deploy $DST_PATH"

    log_v "check $DST_PATH"

    EXISTS=0
    adb_shell "test -f ${DST_PATH}" || EXISTS=1

    if [ $EXISTS -eq 0 ]; then
      log_v "$DST_PATH exists"
      X_DIGEST=$($ADB shell "${XATTR} -n ${XATTR_KEY} ${DST_PATH} | tr -d [:space:] || true")
      DIGEST=$(md5 -q "${SRC}")
      if [ "$DIGEST" != "${X_DIGEST}" ]; then
        log_v "$DST_PATH differs [ '$DIGEST' != '${X_DIGEST}' ]"
        NEEDS_INSTALL=1
      else
        log_v "$DST_PATH up-to-date [$DIGEST]"
      fi
    else
      log_v "$DST_PATH not found"
      NEEDS_INSTALL=1
    fi

    if [ $NEEDS_INSTALL -eq 1 ]; then
      log_v "install $DST_PATH"
      $ADB push "$SRC" "$DST"
      if [ -n $DIGEST ]; then
        DIGEST=$(md5 -q "${SRC}")
      fi
      adb_shell "${XATTR} -n ${XATTR_KEY} -v ${DIGEST} ${DST_PATH}"
    fi
}

function adb_deploy_files()
{
    DST="$1"
    shift
    SRC_LIST=("$@")

    for SRC in "${SRC_LIST[@]}"; do
        echo "deploy $SRC -> $DST"
        adb_deploy $SRC $DST
    done
}

# deploy xattr utility
# $ADB shell test -f ${BIN_INSTALL_PATH}/axattr && $ADB push ${BUILD_ROOT}/bin/axattr ${BIN_INSTALL_PATH}
adb_deploy "${BUILD_ROOT}/bin/axattr" "${BIN_INSTALL_PATH}"

export -f log_v
export -f adb_deploy
export VERBOSE
export ADB
export INSTALL_ROOT
export BIN_INSTALL_PATH
export LIB_INSTALL_PATH
find "${BUILD_ROOT}/lib" -depth 1 -type f -name '*.so' \
    -exec bash -c \
    'adb_deploy "${0}" "${LIB_INSTALL_PATH}"' {} \;

find "${BUILD_ROOT}/bin" -type f -not -name '*.full' -not -name 'axattr' \
     -exec bash -c \
    'adb_deploy "${0}" "${BIN_INSTALL_PATH}"' {} \;

find "${TOPLEVEL}/project/victor/runtime-le" -type f -depth 1 \
    -exec bash -c \
    'adb_deploy "${0}" "${INSTALL_ROOT}/etc"' {} \;

find "${TOPLEVEL}/project/victor/runtime-le/config" -type f -depth 1 \
    -exec bash -c \
    'adb_deploy "${0}" "${INSTALL_ROOT}/etc"' {} \;

# Move the hal.conf file (which contains robot-specific calibration values) to its proper home if needed
HAL_CONF_PATH=/data/persist
$ADB shell mkdir -p $HAL_CONF_PATH
# If hal.conf already exists in com.anki.cozmoengine directory, move it to /data/persist (if folder exists)
adb_shell "test -f ${INSTALL_ROOT}/etc/hal.conf" && $ADB shell mv ${INSTALL_ROOT}/hal.conf $HAL_CONF_PATH

#
# Put a link in /data/appinit.sh for automatic startup
#
$ADB shell ln -sf ${INSTALL_ROOT}/etc/appinit.sh /data/appinit.sh
