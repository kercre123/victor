#!/bin/bash

set -e
set -u
set -x

if [ $# -lt 1 ]
  then
    echo "Usage: $0 API_TOKEN <APP_ID>"
    exit 1
fi

echo "=== Upload to HockeyApp ==="

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
source "${SCRIPT_PATH}/../build_env.sh"

if [ $# -eq 2 ]; then
    APP_ID=$2
fi

if [ -z ${APP_ID+x} ] && [ ! -z ${ANKI_BUILD_HOCKEYAPP_APP_ID+x} ]; then
    APP_ID=${ANKI_BUILD_HOCKEYAPP_APP_ID}
fi

# Set OVERRIDE before calling script to fetch ipa's from a different location.
if [ ! -z ${BUILD_PRODUCTS_PATH_OVERRIDE+x} ]; then
    BUILT_PRODUCTS_PATH=${BUILD_PRODUCTS_PATH_OVERRIDE}
fi

if [ -z ${APP_ID+x} ]; then
    echo "ERROR: Missing HOCKEYAPP_APP_ID"
    exit 1
fi

pushdir ${BUILT_PRODUCTS_PATH}

echo "APP_ID: ${APP_ID}"
${ANKI_BUILD_TOOLS_ROOT}/upload-to-hockeyapp.sh ${1} ${APP_ID} "${BUILT_PRODUCTS_PATH}/${ANKI_BUILD_BUNDLE_NAME}.ipa"  "${BUILT_PRODUCTS_PATH}/${ANKI_BUILD_BUNDLE_NAME}.app.dSYM.zip"

popdir
