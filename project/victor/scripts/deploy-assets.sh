#!/bin/bash

set -e
set -u

# Go to directory of this script                                                                    
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))            
GIT=`which git`                                                                                     
if [ -z $GIT ]                                                                                      
then                                                                                                
    echo git not found                                                                              
    exit 1                                                                                          
fi                                                                                                  
TOPLEVEL=`$GIT rev-parse --show-toplevel`                                                           

function usage()
{
    SCRIPT_NAME=`basename $0`
    echo "${SCRIPT_NAME} [OPTIONS] <ASSETS_BUILD_DIR>"
    echo "  -h          print this message"
    echo "  -f          force-push assets to device"
    echo "  -x          remove all assets bundles from device"
    echo "${SCRIPT_NAME} with no arguments lists the available local assets"
}

#
# defaults
#
FORCE_PUSH_ASSETS=0
REMOVE_ALL_ASSETS=0

while getopts ":hfx" opt; do
  case ${opt} in
    h )
      usage
      exit 1
      ;;
    f )
      FORCE_PUSH_ASSETS=1
      ;;
    x )
      REMOVE_ALL_ASSETS=1
      ;;
    \? )
      usage
      exit 1
      ;;
  esac
done
                                                                                                    
cd "${SCRIPT_PATH}"
shift $((OPTIND -1))


#
# Find asset directories
#
if [ $# -ne 0 ]; then
    ASSETSDIR="${@: -1}" # last argument
else
    ASSETDIRS=($(find ${TOPLEVEL}/_build/android -path '**/assets/cozmo_assets.ref'))
    if [ "${#ASSETDIRS[@]}" -eq 1 ]; then
        ASSETSDIR=$(dirname "${ASSETDIRS[0]}")
    else
        usage
        echo ""
        echo "List of asset build dirs (new -> old)"
        # find asset dirs
        ASSETDIRS=($(find ${TOPLEVEL}/_build/android -path '**/assets/cozmo_assets.ref'))
        if [ "${#ASSETDIRS[@]}" -eq 0 ]; then
            exit 1
        fi

        CONTENTS=($(cat "${ASSETDIRS[@]}"))
        #for idx in "${!ASSETDIRS[@]}"; do
        #    echo "$((idx+1)): ${ASSETDIRS[idx]}"
        #done
        LISTING=($(ls -1t "${ASSETDIRS[@]}"))
        for idx in "${!LISTING[@]}"; do
            DIR_NAME=$(dirname ${LISTING[idx]})
            echo "$((idx+1)): [${CONTENTS[idx]}] ${DIR_NAME}"
        done
        #echo "${CONTENTS[@]}"
        #echo "${LISTING[@]}"
        #echo "${CONTENTS[@]} ${LISTING[@]}" | paste - -
        #echo "${CONTENTS[@]} ${LISTING[@]}" | paste -s -d : - -
        exit 1
    fi
fi

# Check whether an existing hash already exists
SOURCE_ASSET_HASH_FILE="${ASSETSDIR}/cozmo_assets.ref"

if [ ! -f $SOURCE_ASSET_HASH_FILE ]; then
    echo "Missing assets hash file: $SOURCE_ASSET_HASH_FILE"
    echo "rebuild assets."
    exit 1
fi

SOURCE_HASH=$(cat ${SOURCE_ASSET_HASH_FILE})

DEVICE_ASSET_ROOT_DIR="/data/data/com.anki.cozmoengine/files/assets"
DEVICE_ASSET_DIR="$DEVICE_ASSET_ROOT_DIR/$SOURCE_HASH"
DEVICE_ASSET_TMP_DIR="$DEVICE_ASSET_ROOT_DIR/_tmp"

# source adb env & helper functions
source android_env.sh


# delete all old bundles from asset folder if REMOVE_ALL_ASSETS=1
if [ $REMOVE_ALL_ASSETS -eq 1 ]; then
  $ADB shell rm -rf ${DEVICE_ASSET_ROOT_DIR}
# blow away current assets folder to force re-push if FORCE_PUSH_ASSETS=1
elif [ $FORCE_PUSH_ASSETS -eq 1 ]; then
  $ADB shell rm -rf ${DEVICE_ASSET_DIR}
fi

set +e
$ADB shell [ -f "$DEVICE_ASSET_DIR/cozmo_assets.ref" ]
HAS_ASSETS=$?
set -e

if [ $HAS_ASSETS -eq 0 ]; then
    echo "assets available at $DEVICE_ASSET_DIR"
    exit 0
fi

# Install new assets. Deploy to a tmp directory first
# in case transfer is stopped midway through.
echo "deploying assets"
pushd ${ASSETSDIR} > /dev/null 2>&1
$ADB shell rm -rf ${DEVICE_ASSET_DIR} ${DEVICE_ASSET_TMP_DIR} # make sure there is no existing assets directory or tmp directory
$ADB push . ${DEVICE_ASSET_TMP_DIR}/
$ADB shell mv ${DEVICE_ASSET_TMP_DIR} ${DEVICE_ASSET_DIR}
$ADB push ${ASSETSDIR}/cozmo_assets.ref ${DEVICE_ASSET_ROOT_DIR}/current
popd > /dev/null 2>&1
echo "assets installed to $DEVICE_ASSET_DIR"

# reap old assets if there are more than 5 versions
# listing is oldest -> newest
DEVICE_DIR_LIST=($($ADB shell ls -1tr $DEVICE_ASSET_ROOT_DIR))

_MAX_DIRS=5
_COUNT=0
for dir in "${DEVICE_DIR_LIST[@]}"
do
    name=`basename $dir`
    if [ "$name" == ${SOURCE_HASH} ]; then
        continue
    fi
    if [ $_COUNT -ge $_MAX_DIRS ]; then
        old_dir="${DEVICE_ASSET_ROOT_DIR}/$dir"
        echo "remove old assets dir: $old_dir"
        $ADB shell rm -rf ${old_dir}
    fi
    _COUNT=$((_COUNT+1))
done
