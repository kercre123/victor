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

if [ -z ${APP_ID+x} ]; then
    echo "ERROR: Missing HOCKEYAPP_APP_ID_ANDROID"
    exit 1
fi

APK_FILE_NAME="Cozmo"

echo "${APP_ID}"
${ANKI_BUILD_TOOLS_ROOT}/upload-to-hockeyapp.sh ${1} ${APP_ID} "${ANKI_BUILD_ROOT}/android/${APK_FILE_NAME}.apk" "${ANKI_BUILD_ROOT}/android/${APK_FILE_NAME}.apk-symbols.zip"
