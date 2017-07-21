#!/bin/bash

set -e
set -u

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi

TOPLEVEL=`$GIT rev-parse --show-toplevel`



if [ -z ${SDK_PLATFORM+x} ]; then  SDK_PLATFORM="iphoneos"; fi
if [ -z ${BUILD_OS+x} ]; then BUILD_OS="ios"; fi
if [ -z ${l_OS+x} ]; then l_OS=`echo ${BUILD_OS} | awk '{print tolower($0)}'`; fi
if [ -z ${u_OS+x} ]; then u_OS=`echo ${BUILD_OS} | awk '{print toupper($0)}'`; fi

#
# Verify ANKI_BUILD_* environment
#

if [ -z ${ANKI_REPO_ROOT+x} ]; then ANKI_REPO_ROOT=${TOPLEVEL}; fi
if [ -z ${ANKI_BUILD_TOOLS_ROOT+x} ]; then ANKI_BUILD_TOOLS_ROOT="${TOPLEVEL}/project/build-scripts"; fi
if [ -z ${ANKI_BUILD_ROOT+x} ]; then ANKI_BUILD_ROOT="${ANKI_REPO_ROOT}/build"; fi
if [ -z ${ANKI_SDK_ROOT+x} ]; then ANKI_SDK_ROOT="${ANKI_REPO_ROOT}/tools/sdk"; fi
if [ -z ${ANKI_SDK_DEV_ROOT+x} ]; then ANKI_SDK_DEV_ROOT="${ANKI_REPO_ROOT}/tools/sdk_devonly"; fi

if [ -z ${ANKI_BUILD_CONFIGURATION+x} ]; then ANKI_BUILD_CONFIGURATION="release"; fi
if [ -z ${ANKI_BUILD_KEYCHAIN+x} ]; then ANKI_BUILD_KEYCHAIN="${HOME}/Library/Keychains/login.keychain"; fi
if [ -z ${ANKI_BUILD_PROVISIONING_PROFILE_NAME+x} ]; then ANKI_BUILD_PROVISIONING_PROFILE_NAME="Cozmo"; fi
if [ -z ${ANKI_BUILD_XCWORKSPACE+x} ]; then ANKI_BUILD_XCWORKSPACE="${ANKI_REPO_ROOT}/generated/${l_OS}/CozmoWorkspace_${l_OS}.xcworkspace"; fi
if [ -z ${ANKI_BUILD_TYPE+x} ]; then ANKI_BUILD_TYPE="dev"; fi
if [ -z ${ANKI_BUILD_BUNDLE_NAME+x} ]; then ANKI_BUILD_BUNDLE_NAME="Cozmo"; fi
if [ -z ${ANKI_BUILD_VERSION+x} ]; then ANKI_BUILD_VERSION="0"; fi
if [ -z ${ANKI_DAS_VERSION_FILE+x} ]; then ANKI_DAS_VERSION_FILE="${ANKI_BUILD_ROOT}/das_version.log"; fi


#
# iOS specific variables.
#

MP_PARSE="${TOPLEVEL}/project/ios/ProvisioningProfiles/mpParse"
PLIST_BUDDY="/usr/libexec/PlistBuddy"

# build info
XC_WORKSPACE_NAME=`basename "${ANKI_BUILD_XCWORKSPACE}" .xcworkspace`


GENERATED_DD_FOLDER=`find ${ANKI_REPO_ROOT} -type d -name "${XC_WORKSPACE_NAME}-*" | xargs basename`
echo ${GENERATED_DD_FOLDER}
DERIVED_DATA_PATH=${ANKI_BUILD_ROOT}/${l_OS}/derived-data/${GENERATED_DD_FOLDER}

BUILT_PRODUCTS_PATH=${DERIVED_DATA_PATH}/Build/Products/${ANKI_BUILD_CONFIGURATION}-${SDK_PLATFORM}

PROVISIONING_PROFILE="${ANKI_REPO_ROOT}/project/ios/ProvisioningProfiles/${ANKI_BUILD_PROVISIONING_PROFILE_NAME}.mobileprovision"
INFO_PLIST="${ANKI_REPO_ROOT}/unity/ios/Info.plist"

DIST_ROOT="${ANKI_BUILD_ROOT}/dist"


# Dump ANKI_BUILD variables
( set -o posix ; set ) | grep "ANKI_BUILD"



#
# TeamCity parameters
#

: ${TEAMCITY_SERVER_URL:="https://teamcity.ankicore.com"}
: ${TEAMCITY_AUTH_USER_ID:=""}
: ${TEAMCITY_AUTH_PASSWORD:=""}
: ${TEAMCITY_CURL_USER_ARG:=""}
: ${TEAMCITY_BUILD_STEP_NAME:=""}
: ${BUILD_NUMBER:="LOCAL"}
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
