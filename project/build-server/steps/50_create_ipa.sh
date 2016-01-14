#!/bin/bash

set -e
set -u
set -x

echo "=== Build IPA ==="

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
source "${SCRIPT_PATH}/../build_env.sh"


### SET BY BUILD SERVER ###
#ANKI_BUILD_ROOT
#ANKI_BUILD_CONFIGURATION
#ANKI_PROVISIONING_PROFILE
#ANKI_BUILD_BUNDLE_NAME

### PASSED IN ####
#BUILT_PRODUCTS_PATH
# ANKI_BUILD_VERSION
# SDK_PLATFORM


### ASSUMPTIONS ###
# keychain is uplocked.



APP_BUNDLE="${BUILT_PRODUCTS_PATH}/${ANKI_BUILD_BUNDLE_NAME}.app"

ARTIFACT_ROOT=${ANKI_BUILD_ROOT}/dist/ipa
mkdir -p "${ARTIFACT_ROOT}"

IPA_FILE="${ANKI_BUILD_BUNDLE_NAME}-${ANKI_BUILD_CONFIGURATION}-${ANKI_BUILD_PROVISIONING_PROFILE_NAME}-${ANKI_BUILD_VERSION}.ipa"

# create ipa
rm -f "${BUILT_PRODUCTS_PATH}/${ANKI_BUILD_BUNDLE_NAME}.ipa"
rm -f "${ARTIFACT_ROOT}/${IPA_FILE}"

xcrun \
    -sdk ${SDK_PLATFORM} \
    PackageApplication \
    -v ${APP_BUNDLE} \
    -o "${BUILT_PRODUCTS_PATH}/${ANKI_BUILD_BUNDLE_NAME}.ipa"

cp -p "${BUILT_PRODUCTS_PATH}/${ANKI_BUILD_BUNDLE_NAME}.ipa" "${ARTIFACT_ROOT}/${IPA_FILE}"

# archive symbols
SYMBOLS_ARCHIVE_FILE="${ANKI_BUILD_BUNDLE_NAME}.app.dSYM-${ANKI_BUILD_CONFIGURATION}-${ANKI_BUILD_PROVISIONING_PROFILE_NAME}-${ANKI_BUILD_VERSION}.zip"

pushdir ${BUILT_PRODUCTS_PATH}

rm -f "${ANKI_BUILD_BUNDLE_NAME}.app.dSYM.zip"
rm -f "${ARTIFACT_ROOT}/${SYMBOLS_ARCHIVE_FILE}"

if [ -d ${ANKI_BUILD_BUNDLE_NAME}.app.dSYM ]; then
    zip --quiet -r -X ${ANKI_BUILD_BUNDLE_NAME}.app.dSYM.zip ${ANKI_BUILD_BUNDLE_NAME}.app.dSYM
    cp -p ${ANKI_BUILD_BUNDLE_NAME}.app.dSYM.zip "${ARTIFACT_ROOT}/${SYMBOLS_ARCHIVE_FILE}"

    echo "Archived debug symbols to: ${ARTIFACT_ROOT}/${SYMBOLS_ARCHIVE_FILE}"
else
    echo "No debug symbols to archive"
fi

popdir
