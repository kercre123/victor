#!/bin/bash
set -xe

# archive APK
echo "=== Archive symbols ==="

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
source "${SCRIPT_PATH}/../build_env.sh"

#TODO: Pass this into destinquish between release and debug
APK_FILE_NAME=Cozmo.apk

SYMBOL_STORE=${ANKI_BUILD_ROOT}/android/symbols
mkdir -p "${SYMBOL_STORE}"

cp -p ./build/android/libs-prestrip/armeabi-v7a/*.so ${SYMBOL_STORE}

# archive debug symbols
SYMBOLS_ARCHIVE_FILE="${APK_FILE_NAME}-symbols.zip"

zip --quiet -j "${ANKI_BUILD_ROOT}/android/${SYMBOLS_ARCHIVE_FILE}" ${SYMBOL_STORE}/*.so
echo "Archived debug symbols to: ${ANKI_BUILD_ROOT}/android/${SYMBOLS_ARCHIVE_FILE}"
