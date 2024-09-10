#!/bin/bash

set -e
set -u

SRCDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

VERSION_PATH="${SRCDIR}/SDK_VERSION"

: ${VERBOSE:=0}

: ${VERSION:=`awk /version/'{printf "%s", $2}' ${VERSION_PATH}`}
: ${URL:=`awk /url/'{printf "%s", $2}' ${VERSION_PATH}`}
: ${MD5:=`awk /MD5/'{printf "%s", $2}' ${VERSION_PATH}`}

function logv {
    if [ $VERBOSE -eq 1 ]; then
        echo "$*"
    fi
}

logv "WWISE VERSION: $VERSION [ $URL ]"

DEFAULT_WWISE_HOME="${HOME}/.anki/wwise/versions"

: ${ANKI_BUILD_WWISE_HOME:=$DEFAULT_WWISE_HOME}
: ${ANKI_WWISE_SDK_ROOT:="${ANKI_BUILD_WWISE_HOME}/${VERSION}"}
: ${WWISE_SDK_ROOT:="${ANKI_WWISE_SDK_ROOT}"}

logv "WWISE_SDK_ROOT = ${WWISE_SDK_ROOT}"

NEEDS_INSTALL=0

if [ ! -d "${WWISE_SDK_ROOT}" ]; then
    mkdir -p "${WWISE_SDK_ROOT}"
    NEEDS_INSTALL=1
fi

VERSION_H_REL="include/AK/AkWwiseSDKVersion.h"
VERSION_H_PATH="${WWISE_SDK_ROOT}/${VERSION_H_REL}"

if [ -f "${VERSION_H_PATH}" ]; then
    MAJOR=`awk '/^#define AK_WWISESDK_VERSION_MAJOR/{printf "%d", $3}' ${VERSION_H_PATH}`
    MINOR=`awk '/^#define AK_WWISESDK_VERSION_MINOR/{printf "%d", $3}' ${VERSION_H_PATH}`
    PATCH=`awk '/^#define AK_WWISESDK_VERSION_SUBMINOR/{printf "%d", $3}' ${VERSION_H_PATH}`
    logv "SDK HEADER VERSION: $MAJOR $MINOR $PATCH"
else
    NEEDS_INSTALL=1
fi

logv "Install required? $NEEDS_INSTALL"

if [ $NEEDS_INSTALL -eq 1 ]; then
    DROPBOX_ARCHIVE_PATH="${HOME}/Dropbox\ \(Anki\,\ Inc\)/Anki\ Software\ Engineering/dist/wwise/wwise-$VERSION.tar.bz2"

    if [ -f "${DROPBOX_ARCHIVE_PATH}" ]; then
        ARCHIVE_PATH="${DROPBOX_ARCHIVE_PATH}"
    fi

    DL_ARCHIVE_PATH="${HOME}/Downloads/wwise-$VERSION.tar.bz2"
    mkdir -p `dirname ${DL_ARCHIVE_PATH}`
    : ${ARCHIVE_PATH:="${DL_ARCHIVE_PATH}"}

    WWISE_ARCHIVE="${ARCHIVE_PATH}"


    [ -f "${WWISE_ARCHIVE}" ] && NEEDS_WWISE_PACKAGE=0 || NEEDS_WWISE_PACKAGE=1

    PACKAGE_VERIFIED=0
    TRIAL_NUMBER=0
    while [ $PACKAGE_VERIFIED -eq 0 -a $TRIAL_NUMBER -lt 3 ]; do
        TRIAL_NUMBER=$((TRIAL_NUMBER+1))
        if [ -f "${WWISE_ARCHIVE}" ]; then
            logv "Confirming "${WWISE_ARCHIVE}" has proper md5 checksum..."
            logv "openssl md5 "${WWISE_ARCHIVE}
            MD5_OUTPUT=$(openssl md5 ${WWISE_ARCHIVE})
            GENERATED_MD5=$(echo $MD5_OUTPUT | awk '{print $NF}')
            echo "MD5: $GENERATED_MD5"
            if [ "${GENERATED_MD5}" == "${MD5}" ]; then
                PACKAGE_VERIFIED=1
            else
                echo "MD5 checksum of "${WWISE_ARCHIVE}" did not match with checksum in "${VERSION_PATH}
                echo "Assuming corrupted; re-fetching file from url in "${VERSION_PATH}

                rm $WWISE_ARCHIVE
            fi
        fi

        if [ ! -f "${WWISE_ARCHIVE}" ]; then
            logv "Fetch Wwise Archive [ $URL ]"
            logv "curl -o "${DL_ARCHIVE_PATH}" "${URL}""
            curl -L -o "${WWISE_ARCHIVE}" "${URL}"
        fi
    done
    

    logv "tar xjvf "${WWISE_ARCHIVE}" -C "${WWISE_SDK_ROOT}" --strip-components 1"
    tar xjvf "${WWISE_ARCHIVE}" -C "${WWISE_SDK_ROOT}" --strip-components 1
    chmod -R 555 "${WWISE_SDK_ROOT}"
    find "${WWISE_SDK_ROOT}/../" -type d -exec chmod 755 {} \;
    echo "Wwise installed at ${WWISE_SDK_ROOT}"
else
    echo "Wwise installed at ${WWISE_SDK_ROOT}"
fi

# check if wwise sdk path enviroment var is set, if not use default
WWISE_SDK_SYMLINK=${WWISE_SDK_PATH:-${WWISE_SDK_ROOT}}

# symlink to current
LOCAL_CURRENT_LINK="${SRCDIR}/versions/current"
mkdir -p "${SRCDIR}/versions"
if [ "$(readlink $LOCAL_CURRENT_LINK)" != "$WWISE_SDK_SYMLINK" ]; then
    rm -f "${LOCAL_CURRENT_LINK}"
    ln -s "${WWISE_SDK_SYMLINK}" "${LOCAL_CURRENT_LINK}"
    echo "Symlink ${LOCAL_CURRENT_LINK} -> ${WWISE_SDK_SYMLINK}"
else
    logv "${LOCAL_CURRENT_LINK} exists"
fi

