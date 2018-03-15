#!/bin/bash
set -x
set -u
set -e

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))

GIT=`which git`
if [ -z $GIT ]; then
  echo git not found
  exit 1
fi
: ${TOPLEVEL:=`$GIT rev-parse --show-toplevel`}

function vlog()
{
    echo "[fetch-build-deps] $*"
}

pushd "${TOPLEVEL}" > /dev/null 2>&1

$GIT config --global url."git@github.com:".insteadOf https://github.com

OS_NAME=$(uname -s)
case $OS_NAME in
    "Darwin")
        HOST="mac"
        ;;
    "Linux")
        HOST="linux"
        ;;
esac

HOST_FETCH_DEPS=${SCRIPT_PATH}/"fetch-build-deps.${HOST}.sh"

if [ -x "${HOST_FETCH_DEPS}" ]; then
    ${HOST_FETCH_DEPS}
else
    echo "ERROR: Could not determine platform for system name: $OS_NAME"
    exit 1
fi
