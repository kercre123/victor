#!/bin/bash

set -e

: ${TEAMCITY_BUILD_WORKING_DIR:=""}
: ${ANKI_BUILD_HOCKEYAPP_APP_ID:=""}

if [ -z "${TEAMCITY_BUILD_WORKING_DIR}" -a ! -z "${ANKI_BUILD_HOCKEYAPP_APP_ID}" ]; then
  echo "Script is being run outside of the build server or with a misconfigured TEAMCITY configuration."
  exit 1
fi

BUILD_PRODUCTS_PATH_OVERRIDE=${TEAMCITY_BUILD_WORKING_DIR}/rewrap ./project/build-server/steps/60_upload_ipa_to_hockeyapp.sh b0cc3e2cf3b242f393149aa5f901954d ${ANKI_BUILD_HOCKEYAPP_APP_ID}

