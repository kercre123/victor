#!/bin/bash

set -e
set -u

echo "=== Updating Android Manifest ==="

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
source "${SCRIPT_PATH}/../build_env.sh"

# tools
VERSION_GENERATOR="${ANKI_BUILD_TOOLS_ROOT}/version-generator.py"

#regex
VERSION_LINE="\(bundleVersion:\)\(.*\)"
DAS_VERSION_LINE="\(<meta-data android:name=\"dasVersionName\" android:value=\"\)\(.*\)\(\" \/>\)"


MANIFEST=./unity/Cozmo/Assets/Plugins/Android/Cozmo/AndroidManifest.xml


pushdir ${ANKI_REPO_ROOT}


# commenting out DAS versioning since it's
# a.) currently not used in cozmo (but probably will be in the future)
# b.) the script fails to run due to SETTINGS_FILE not being defined with cozmo's
#     current environment setup
# MARKETING_VERSION=$(grep $VERSION_LINE $SETTINGS_FILE | sed "s@$VERSION_LINE@\2@" )

# DAS_VERSION=`${VERSION_GENERATOR} \
#     --build-version ${ANKI_BUILD_VERSION} \
#     --build-type ${ANKI_BUILD_TYPE} \
#     ${MARKETING_VERSION}`

# echo "Inserting das.version $DAS_VERSION in $MANIFEST"

# sed -i '' "s@$DAS_VERSION_LINE@\1$DAS_VERSION\3@" $MANIFEST

: ${ANKI_BUILD_HOCKEYAPP_APP_ID:=""}
# insert hockeyapp id
if [ -n "$ANKI_BUILD_HOCKEYAPP_APP_ID" ]; then
    echo "Inserting hockeyapp id $ANKI_BUILD_HOCKEYAPP_APP_ID in $MANIFEST"
    HOCKEYAPP_ID_LINE="\(<meta-data android:name=\"HOCKEYAPP_APP_ID\" android:value=\"\)\(.*\)\(\" \/>\)"
    sed -i '' "s@$HOCKEYAPP_ID_LINE@\1$ANKI_BUILD_HOCKEYAPP_APP_ID\3@" $MANIFEST
fi

popdir
