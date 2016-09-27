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

unzip ${APK_LOCATION}/${APK_NAME} -d tmp

cd tmp 
rm -rf META-INF
zip -r cozmo_zip.apk *
mv cozmo_zip.apk ..
cd ..
rm -rf tmp

mkdir -p build/android/signed_apk

jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore ${ANKI_BUILD_KEYSTORE_PATH} cozmo_zip.apk ankirelease -storepass ${KEY_STORE_PWD}
zipalign -f -v 4 cozmo_zip.apk build/android/signed_apk/${APK_NAME}
rm cozmo_zip.apk
