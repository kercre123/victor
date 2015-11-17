#!/bin/bash

set -e
set -u

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

# System tools


#
# Verify ANKI_BUILD_* environment
#

if [ -z ${ANKI_REPO_ROOT+x} ]; then ANKI_REPO_ROOT=${TOPLEVEL}; fi
if [ -z ${ANKI_BUILD_TOOLS_ROOT+x} ]; then ANKI_BUILD_TOOLS_ROOT="${TOPLEVEL}/project/build-scripts"; fi
if [ -z ${ANKI_BUILD_ROOT+x} ]; then ANKI_BUILD_ROOT="${ANKI_REPO_ROOT}/build"; fi

#if [ -z ${ANKI_BUILD_CONFIGURATION+x} ]; then ANKI_BUILD_CONFIGURATION="Release"; fi
if [ -z ${ANKI_BUILD_KEYCHAIN+x} ]; then ANKI_BUILD_KEYCHAIN="${HOME}/Library/Keychains/login.keychain"; fi
#if [ -z ${ANKI_BUILD_PROVISIONING_PROFILE_NAME+x} ]; then ANKI_BUILD_PROVISIONING_PROFILE_NAME="Cozmo"; fi
#if [ -z ${ANKI_BUILD_XCWORKSPACE+x} ]; then ANKI_BUILD_XCWORKSPACE="${ANKI_REPO_ROOT}/generated/Cozmo-iOS.xcworkspace"; fi
#if [ -z ${ANKI_BUILD_TYPE+x} ]; then ANKI_BUILD_TYPE="dev"; fi
#if [ -z ${ANKI_BUILD_BUNDLE_NAME+x} ]; then ANKI_BUILD_BUNDLE_NAME="Cozmo"; fi
#if [ -z ${ANKI_BUILD_PREBUILT_BS_ROOT+x} ]; then ANKI_BUILD_PREBUILT_BS_ROOT="${TOPLEVEL}/basestation-prebuilt"; fi

#
# TeamCity parameters
#

: ${TEAMCITY_SERVER_URL:="http://192.168.2.3:8111"}
: ${TEAMCITY_AUTH_USER_ID:=""}
: ${TEAMCITY_AUTH_PASSWORD:=""}
: ${TEAMCITY_CURL_USER_ARG:=""}

if [ -z "${TEAMCITY_CURL_USER_ARG}" -a ! -z "${TEAMCITY_AUTH_USER_ID}" ]; then
    TEAMCITY_CURL_USER_ARG="--user ${TEAMCITY_AUTH_USER_ID}"
    if [ ! -z "${TEAMCITY_AUTH_PASSWORD}" ]; then
        TEAMCITY_CURL_USER_ARG="${TEAMCITY_CURL_USER_ARG}:${TEAMCITY_AUTH_PASSWORD}"
    fi
fi

#
# helper functions
#

function pushdir() {
    pushd $1 > /dev/null 2>&1
}

function popdir() {
    popd > /dev/null 2>&1
}
