#!/bin/bash

set -e
set -u

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

_SCRIPT_PATH=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)
if [ -f ${_SCRIPT_PATH}/../build_env.sh ]; then
  source ${_SCRIPT_PATH}/../build_env.sh
fi

# Derived Info used by build scripts

# tools
MP_PARSE="${ANKI_BUILD_TOOLS_ROOT}/../mobile_provisioning/mpParse"
PLIST_BUDDY="/usr/libexec/PlistBuddy"

# build info
XC_WORKSPACE_NAME=`basename "${ANKI_BUILD_XCWORKSPACE}" .xcworkspace`
DERIVED_DATA_PATH=${ANKI_BUILD_ROOT}/DerivedData/${XC_WORKSPACE_NAME}
IOS_SDK="iphoneos"

DERIVED_DATA_PATH=${ANKI_BUILD_ROOT}/DerivedData/${XC_WORKSPACE_NAME}
BUILT_PRODUCTS_PATH=${DERIVED_DATA_PATH}/Build/Products/${ANKI_BUILD_CONFIGURATION}-${IOS_SDK}

PROVISIONING_PROFILE="${ANKI_REPO_ROOT}/projects/ios/ProvisioningProfiles/${ANKI_BUILD_PROVISIONING_PROFILE_NAME}.mobileprovision"
INFO_PLIST="${ANKI_REPO_ROOT}/projects/ios/OverDrive/OverDrive/Info.plist"

#DIST_ROOT="${ANKI_BUILD_ROOT}/dist"

if [ -z ${ANKI_BUILD_VERSION+x} ]; then
    ANKI_BUILD_VERSION="0"
fi

# Dump ANKI_BUILD variables
( set -o posix ; set ) | grep "ANKI_BUILD"
