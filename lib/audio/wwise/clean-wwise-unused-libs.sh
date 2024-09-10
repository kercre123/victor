#!/bin/bash

set -e
set -u

SRCDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

VERSION_PATH="${SRCDIR}/SDK_VERSION"

: ${VERBOSE:=0}

: ${VERSION:=`awk /version/'{printf "%s", $2}' ${VERSION_PATH}`}

echo "Version $VERSION"

DEFAULT_WWISE_HOME="${HOME}/.anki/wwise/versions"

: ${ANKI_BUILD_WWISE_HOME:="${DEFAULT_WWISE_HOME}"}
: ${ANKI_WWISE_SDK_ROOT:="${ANKI_BUILD_WWISE_HOME}/${VERSION}"}
: ${WWISE_SDK_ROOT:="${ANKI_WWISE_SDK_ROOT}"}

IS_INSTALLED=0

if [ -d "${WWISE_SDK_ROOT}" ]; then
    IS_INSTALLED=1
fi

VERSION_H_REL="include/AK/AkWwiseSDKVersion.h"
VERSION_H_PATH="${WWISE_SDK_ROOT}/${VERSION_H_REL}"

if [ -f "${VERSION_H_PATH}" ]; then
    MAJOR=`awk '/^#define AK_WWISESDK_VERSION_MAJOR/{printf "%d", $3}' ${VERSION_H_PATH}`
    MINOR=`awk '/^#define AK_WWISESDK_VERSION_MINOR/{printf "%d", $3}' ${VERSION_H_PATH}`
    PATCH=`awk '/^#define AK_WWISESDK_VERSION_SUBMINOR/{printf "%d", $3}' ${VERSION_H_PATH}`
    echo "SDK HEADER VERSION: $MAJOR $MINOR $PATCH"
else
    IS_INSTALLED=1
fi

if [[ $IS_INSTALLED -eq 0 ]]; then
    # Notify user they are missing libs
    echo "ERROR - Can't find current libs"
fi

pushd ${DEFAULT_WWISE_HOME}

for dir in */ ; do
    if [ ${dir} != "$VERSION/" ] && [ ${dir} != "*/" ]; then
        echo "Remove audio libs '$dir'"
        rm -rf $dir
    fi
done

popd
