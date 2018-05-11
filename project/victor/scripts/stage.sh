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
: ${ANKI_BUILD_TYPE:="Debug"}
: ${INSTALL_ROOT:="/anki"}

function usage() {
  echo "$SCRIPT_NAME [OPTIONS]"
  echo "options:"
  echo "  -h                      print this message"
  echo "  -k                      keep existing files in staging dir"
  echo "  -v                      print verbose output"
  echo "  -c CONFIGURATION        build configuration {Debug,Release}"
  echo ""
  echo "environment variables:"
  echo '  $ANKI_BUILD_TYPE        build configuration {Debug,Release}'
  echo '  $BUILD_ROOT             root dir of build artifacts containing {bin,lib,etc,data} dirs'
  echo '  $STAGING_DIR            directory to hold staged artifacts before deploy to robot'
}

KEEPARG=""

while getopts "hkrvc:s:" opt; do
  case $opt in
    h)
      usage && exit 0
      ;;
    k)
      KEEPARG="-k"
      ;;
    v)
      VERBOSE=1
      ;;
    c)
      ANKI_BUILD_TYPE="${OPTARG}"
      ;;
    r)
      ;;
    s)
      ;;
    *)
      usage && exit 1
      ;;
  esac
done

: ${PLATFORM_NAME:="vicos"}
: ${BUILD_ROOT:="${TOPLEVEL}/_build/${PLATFORM_NAME}/${ANKI_BUILD_TYPE}"}
: ${STAGING_DIR:="${TOPLEVEL}/_build/staging/${ANKI_BUILD_TYPE}"}

# install.sh is a script that is run here by stage.sh, but also independently by
# bitbake when building the Victor OS.
${SCRIPT_PATH}/install.sh ${KEEPARG} ${BUILD_ROOT} ${STAGING_DIR}
