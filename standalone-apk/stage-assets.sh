#!/bin/bash

set -e
set -u

SCRIPT_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))            
_SCRIPT_NAME=`basename ${0}`                                                                        
TOPLEVEL="$( cd "${SCRIPT_PATH}/.." && pwd )"

function usage() {
    echo "$_SCRIPT_NAME [OPTIONS]"
    echo "  -h                          print this message"
    echo "  -f                          force assets to be copied"
}

FORCE=0
VERBOSE=0

while getopts ":hfv" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        f)
            FORCE=1
            ;;
        v)
            VERBOSE=1
            ;;
        :)
            echo "Option -${OPTARG} required an argument." >&2
            usage
            exit 1
            ;;
    esac
done

function LOGv()
{
    if [ $VERBOSE -eq 1 ]; then echo "[$_SCRIPT_NAME] $1"; fi
}

ANDROID_ASSETS_DIR=${TOPLEVEL}/standalone-apk/resources/assets
BUILD_ASSETS_DIR=${TOPLEVEL}/standalone-apk/_build/assets
BUILD_ASSETS_DIGEST="${BUILD_ASSETS_DIR}/cozmo_resources/allAssetHash.txt"


mkdir -p "${BUILD_ASSETS_DIR}"

# Running copyResources.py is expensive.
# Check to see whether we have an existing digest file before running.
# Note that this is _not_ a perfect check, since the assets might be out of date.
# However, this will at least save time on repeated incremental builds from 
# configure.py, which calls this script.

if [ $FORCE -eq 1 ] || [ ! -f ${BUILD_ASSETS_DIGEST} ]; then

[ $FORCE -eq 1 ] && LOGv "Force asset copy"

LOGv "Run copyResources.py"

./project/build-scripts/copyResources.py \
    --destination $BUILD_ASSETS_DIR \
    --copyMethod rsync \
    --platform android \
    --buildToolsPath tools/build/ \
    --externalAssetsPath EXTERNALS/cozmo-assets/ \
    --animationGroupsPath resources/assets/animationGroupMaps/ \
    --cubeAnimationGroupPath resources/assets/cubeAnimationGroupMaps/ \
    --dailyGoalsPath resources/assets/DailyGoals/ \
    --rewardedActionsPath resources/assets/RewardedActions/ \
    --engineResourcesPath resources/config/ \
    --unityAssetsPath unity/Cozmo/Assets/StreamingAssets/ \
    --ttsResourcesPath generated/android/resources/tts/

    # Do not copy sound banks
    #--soundBanksPath EXTERNALS/cozmosoundbanks/GeneratedSoundBanks \

fi


pushd "${BUILD_ASSETS_DIR}" > /dev/null 2>&1

#
# cleanup
#

LOGv "Remove unecessary resources"

# Remove Unity AssetBundles (if present)
rm -rf AssetBundles

# remove sound (if present)
rm -rf cozmo_resources/sound

#
# calculate a hash of the contents of ${BUILD_ASSETS_DIR}
#
# Cozmo app uses algorithm in:
# unity/Cozmo//Assets/Plugins/Libraries/Anki/Build/Editor/Builder.cs

LOGv "Calculate hash digest of all assets"

SESSION_KEY="$$"
RESOURCES_FILE="/tmp/resources.${SESSION_KEY}.txt"
HASHES_FILE="/tmp/resources.hash_${SESSION_KEY}.txt"

find . -not -name '.' -print | sed -e 's/^\.\///g' > $RESOURCES_FILE

IFS=$'\n'       # make newlines the only separator
set -f          # disable globbing
MD5_FILELIST=()
for i in $(cat < "$RESOURCES_FILE"); do
  if [ -f "$i" ]; then
    MD5_FILELIST+=("$i")
  fi
done

# calculate hashes of for each file in the list
md5 -q "${MD5_FILELIST[@]}" > "${HASHES_FILE}"

# combine all filenames and hashes
CONTENTS_FILE="/tmp/resources.contentshash_${SESSION_KEY}.txt"
tr -d '\n' < ${RESOURCES_FILE} >> ${CONTENTS_FILE}
tr -d '\n' < ${HASHES_FILE} >> ${CONTENTS_FILE}

md5 -q ${CONTENTS_FILE} > "cozmo_resources/allAssetHash.txt"

rm -f "${RESOURCES_FILE}"
rm -f "${HASHES_FILE}"
rm -f "${CONTENTS_FILE}"


# regenerate resources.txt manifest
LOGv "Generate resources.txt manifest"

$(cd ${BUILD_ASSETS_DIR} && find . -not -name '.' | sed -e 's/^\.\///g' | sort > ${BUILD_ASSETS_DIR}/resources.txt)

popd > /dev/null 2>&1

#
# Write hash to file to link app to asset contents
#

LOGv "Write hash digest to ${ANDROID_ASSETS_DIR}/cozmo_assets.ref"
mkdir -p ${ANDROID_ASSETS_DIR}
cp -p ${BUILD_ASSETS_DIR}/cozmo_resources/allAssetHash.txt ${ANDROID_ASSETS_DIR}/cozmo_assets.ref
