#!/bin/bash

set -e
set -u
set -x

if [ $# -lt 1 ]
  then
    echo "Usage: $0 API_TOKEN <APP_ID>"
    exit 1
fi

echo "=== Upload Apk to HockeyApp ==="

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
source "${SCRIPT_PATH}/../build_env.sh"

if [ $# -eq 2 ]; then
    APP_ID=$2
fi

if [ -z ${APP_ID+x} ] && [ ! -z ${ANKI_BUILD_HOCKEYAPP_APP_ID_ANDROID+x} ]; then
    APP_ID=${ANKI_BUILD_HOCKEYAPP_APP_ID_ANDROID}
fi

# Set OVERRIDE before calling script to fetch apk's from a different location.
if [ ! -z ${BUILD_PRODUCTS_PATH_OVERRIDE+x} ]; then
    BUILT_PRODUCTS_PATH=${BUILD_PRODUCTS_PATH_OVERRIDE}
fi

if [ -z ${APP_ID+x} ]; then
    echo "ERROR: Missing HOCKEYAPP_APP_ID_ANDROID"
    exit 1
fi

#if [ ${ANKI_BUILD_TYPE} == "Release" ]; then
  APK_FILE_NAME="Cozmo"
#else
#  APK_FILE_NAME="Cozmo-${ANKI_BUILD_TYPE}"
#fi

echo "${APP_ID}"
${ANKI_BUILD_TOOLS_ROOT}/upload-to-hockeyapp.sh ${1} ${APP_ID} "${ANKI_BUILD_ROOT}/android/${APK_FILE_NAME}.apk" "${ANKI_BUILD_ROOT}/android/${APK_FILE_NAME}.apk-symbols.zip"
