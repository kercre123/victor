#!/bin/bash
set -e
set -u
# This script does a resigning of an android apk build.
# It should be replaced with building with the right certs.

APK_LOCATION=$1
APK_NAME=$2
# XXX: convert fetchPassword from build.gradle in OD to BUCK 
KEY_STORE_PWD=$3

: ${ANKI_BUILD_KEYSTORE_PATH:=""}

if [ -z "${ANKI_BUILD_KEYSTORE_PATH}" ]; then
  echo "Script is being run outside of the build server or with a misconfigured TEAMCITY configuration."
  exit 1
fi

if [ ! -d "${APK_LOCATION}" -o ! -f "${APK_LOCATION}/${APK_NAME}" ]; then
  echo "Script was passed in nonexistent file parameters."
  exit 1
fi

zip -d ${APK_LOCATION}/${APK_NAME} "META-INF/*"

mkdir -p build/android/signed_apk

jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore ${ANKI_BUILD_KEYSTORE_PATH} ${APK_LOCATION}/${APK_NAME} ankirelease -storepass ${KEY_STORE_PWD}
zipalign -f -v 4 ${APK_LOCATION}/${APK_NAME} ${APK_LOCATION}/${APK_NAME}.tmp
mv ${APK_LOCATION}/${APK_NAME}.tmp ${APK_LOCATION}/${APK_NAME}
