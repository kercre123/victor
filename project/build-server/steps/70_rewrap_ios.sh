#!/bin/bash
set -x
set -e
set -u

echo  "========= Rewrap IPA ======="

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
source "${SCRIPT_PATH}/../build_env.sh"

: ${ANKI_PROVISIONING_LOCATION:=""}
: ${ANKI_BUILD_PROVISIONING_PROFILE_NAME:=""}
: ${ANKI_BUILD_DISTRIB_SHA:=""}
: ${ANKI_REPO_ROOT=""}
: ${ANKI_PRODUCT="Cozmo"}
: ${ANKI_BUILD_KEYCHAIN="release.keychain"}

PROVISIONING_PROFILE="${ANKI_REPO_ROOT}/project/ios/ProvisioningProfiles/${ANKI_BUILD_PROVISIONING_PROFILE_NAME}.mobileprovision"

if [ -z "${ANKI_BUILD_PROVISIONING_PROFILE_NAME}" -a ! -z "${ANKI_PROVISIONING_LOCATION}" -a ! -z "${ANKI_BUILD_DISTRIB_SHA}" ]; then
  echo "Script is being run outside of the build server or with a misconfigured TEAMCITY configuration."
  exit 1
fi

mkdir -p rewrap

cd rewrap 

# There is an assumption that ${ANKI_PRODUCT}.ipa and ${ANKI_PRODUCT}.app.dSYM.zip is being downloaded from a previous build to the root directory.

rm -fr output
unzip ../ipa/${ANKI_PRODUCT}*.ipa -d output
pwd
ls
cd output

rm -rfv Payload/${ANKI_PRODUCT}.app/_CodeSignature

cp -vf ${PROVISIONING_PROFILE} Payload/${ANKI_PRODUCT}.app/embedded.mobileprovision

# XXX:  Must add a step to generate the entitlements.plist. Stop gap is copying it.
codesign --deep --entitlements=${ANKI_PROVISIONING_LOCATION}/entitlements.plist --verify --no-strict -vvvv -f -s ${ANKI_BUILD_DISTRIB_SHA} --keychain=${ANKI_BUILD_KEYCHAIN} Payload/${ANKI_PRODUCT}.app
zip -r ${ANKI_PRODUCT}.ipa Payload/
codesign --deep --entitlements=${ANKI_PROVISIONING_LOCATION}/entitlements.plist --verify --no-strict -vvvv -f -s ${ANKI_BUILD_DISTRIB_SHA} --keychain=${ANKI_BUILD_KEYCHAIN} ${ANKI_PRODUCT}.ipa
cp -v ${ANKI_PRODUCT}.ipa ../${ANKI_PRODUCT}.ipa

cp ../../symbols/${ANKI_PRODUCT}.app.dSYM*.zip ../${ANKI_PRODUCT}.app.dSYM.zip

echo "done"
