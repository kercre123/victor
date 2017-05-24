#!/bin/bash

set -e
set -x

if test -z "$SCRIPT_ENGINE"
then
    SCRIPTENGINE=il2cpp
else
    SCRIPTENGINE=$SCRIPT_ENGINE
fi

if test -z "$ANKI_BUILD_REPO_ROOT"
then
    echo "No ANKI_BUILD_REPO_ROOT set; expected ios.xcconfig to exist." >&2
    exit 1
fi

CONFIG=$(echo ${CONFIGURATION} | tr '[:upper:]' '[:lower:]')

pwd
echo
python -u "${ANKI_BUILD_REPO_ROOT}/tools/build/tools/ankibuild/unity.py" \
    --platform ios \
    --config "${CONFIG}" \
    --sdk "${PLATFORM_NAME}" \
    --script-engine "${SCRIPTENGINE}" \
    --build-dir "${ANKI_BUILD_UNITY_XCODE_BUILD_DIR}" \
    --log-file "${ANKI_BUILD_UNITY_BUILD_DIR}/UnityBuild-${CONFIG}-${PLATFORM_NAME}.log" \
    --xcode-build-dir "${ANKI_BUILD_UNITY_XCODE_BUILD_DIR}" \
    --with-unity "${ANKI_BUILD_UNITY_EXE}" \
    --build-type OnlyPlayer \
    --verbose \
    "${ANKI_BUILD_UNITY_PROJECT_PATH}"

