#!/bin/bash

set -e
set -u

echo "=== Configure Info.plist ==="

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
source "${SCRIPT_PATH}/build_env.sh"

# tools
# VERSION_GENERATOR="${ANKI_BUILD_TOOLS_ROOT}/version-generator.py"

# print environment 
#printenv | grep "ANKI_BUILD"

echo ""
echo "INFO_PLIST: ${INFO_PLIST}"
echo "PROVISIONING_PROFILE: ${PROVISIONING_PROFILE}"
echo ""

pushdir ${ANKI_REPO_ROOT}

#MARKETING_VERSION=`${PLIST_BUDDY} -c "Print :CFBundleShortVersionString" ${INFO_PLIST}`
#DAS_VERSION=`${VERSION_GENERATOR} \
#    --build-version ${ANKI_BUILD_VERSION} \
#    --build-type ${ANKI_BUILD_TYPE} \
#    ${MARKETING_VERSION}`

BUNDLE_ID=`${MP_PARSE} -f ${PROVISIONING_PROFILE} -o bundle_id`

echo "CFBundleName: ${ANKI_BUILD_BUNDLE_NAME}"
echo "CFBundleIdentifier: ${BUNDLE_ID}"
echo "CFBundleVersion: ${ANKI_BUILD_VERSION}"

${PLIST_BUDDY} -c "Set :CFBundleVersion ${ANKI_BUILD_VERSION}" \
               -c "Set :CFBundleIdentifier ${BUNDLE_ID}" \
               -c "Set :CFBundleName ${ANKI_BUILD_BUNDLE_NAME}" \
               -c "Set :com.anki.das.version ${DAS_VERSION}" \
               "${INFO_PLIST}"

if [ ! -z ${ANKI_BUILD_HOCKEYAPP_APP_ID+x} ]; then
    ${PLIST_BUDDY} -c "Set :com.anki.hockeyapp.appid ${ANKI_BUILD_HOCKEYAPP_APP_ID}" "${INFO_PLIST}"
    echo "com.anki.hockeyapp.appid: ${ANKI_BUILD_HOCKEYAPP_APP_ID}"
fi

popdir

