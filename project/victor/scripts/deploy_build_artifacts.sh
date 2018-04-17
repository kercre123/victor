#!/bin/bash

set -e
set -u

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))

TOPLEVEL="${SCRIPT_PATH}" STAGING_DIR="${SCRIPT_PATH}/Release" RSYNC_BIN_DIR="${SCRIPT_PATH}/rsync" ${SCRIPT_PATH}/deploy.sh -r -f "$@"
