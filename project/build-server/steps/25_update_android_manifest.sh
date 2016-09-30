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

#Files
MANIFEST=./unity/Cozmo/Assets/Plugins/Android/Cozmo/AndroidManifest.xml


pushdir ${ANKI_REPO_ROOT}

#VERSION is the master version file at root of project.
MARKETING_VERSION=`cat VERSION`

DAS_VERSION=`${VERSION_GENERATOR} \
     --build-version ${ANKI_BUILD_VERSION} \
     --build-type ${ANKI_BUILD_TYPE} \
     ${MARKETING_VERSION}`

echo "Inserting das.version $DAS_VERSION in $MANIFEST"

sed -i '' "s@$DAS_VERSION_LINE@\1$DAS_VERSION\3@" $MANIFEST

: ${ANKI_BUILD_HOCKEYAPP_APP_ID_ANDROID:=""}
# insert hockeyapp id
if [ -n "$ANKI_BUILD_HOCKEYAPP_APP_ID_ANDROID" ]; then
    echo "Inserting hockeyapp id $ANKI_BUILD_HOCKEYAPP_APP_ID_ANDROID in $MANIFEST"
    HOCKEYAPP_ID_LINE="\(<meta-data android:name=\"HOCKEYAPP_APP_ID\" android:value=\"\)\(.*\)\(\" \/>\)"
    sed -i '' "s@$HOCKEYAPP_ID_LINE@\1$ANKI_BUILD_HOCKEYAPP_APP_ID_ANDROID\3@" $MANIFEST
fi

popdir
