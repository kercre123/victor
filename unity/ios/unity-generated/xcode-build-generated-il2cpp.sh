#!/bin/bash

set -e
set -o pipefail

# Dump ANKI_BUILD variables
( set -o posix ; set ) | grep "ANKI_BUILD"

if test -z "$SCRIPT_ENGINE"
then
    SCRIPTENGINE=il2cpp
else
    SCRIPTENGINE=$SCRIPT_ENGINE
fi

if [ "$SCRIPTENGINE" != "il2cpp" ]; then
    echo "il2cpp not detected, no generated code to compile."
    exit 0
fi

${SRCROOT}/build-unity-generated.sh -c ${ANKI_BUILD_CMAKE_BIN}

SRC_LIB="${ANKI_BUILD_UNITY_XCODE_BUILD_DIR}/build_libunity-generated/libunity-generated.a"
SRC_LIB_DIGEST=""

TARGET_LIB="${BUILT_PRODUCTS_DIR}/${EXECUTABLE_PREFIX}${PRODUCT_NAME}.${EXECUTABLE_EXTENSION}"
TARGET_LIB_DIGEST=""

[ -f ${TARGET_LIB} ] && TARGET_LIB_DIGEST=`/sbin/md5 -q ${TARGET_LIB}`

[ -f ${SRC_LIB} ] && SRC_LIB_DIGEST=`/sbin/md5 -q ${SRC_LIB}`

if [ "${TARGET_LIB_DIGEST}" != "${SRC_LIB_DIGEST}" ]; then
    cp -vp "${SRC_LIB}" "${TARGET_LIB}"
fi
