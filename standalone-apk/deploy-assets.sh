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
                                                                                                    
cd "${SCRIPT_PATH}"

source android_env.sh

ASSETSDIR=`pwd`/_build/assets

if [ ! -d ${ASSETSDIR} ]; then
    "${SCRIPT_PATH}/stage-assets.sh"
fi

# Check whether an existing hash already exists
SOURCE_ASSET_HASH_FILE="${ASSETSDIR}/cozmo_resources/allAssetHash.txt"

if [ ! -f $SOURCE_ASSET_HASH_FILE ]; then
    echo "Missing assets hash file: $SOURCE_ASSET_HASH_FILE"
    echo "re-run ${SCRIPT_PATH}/stage-assets.sh"
    exit 1
fi

SOURCE_HASH=$(cat ${SOURCE_ASSET_HASH_FILE})

DEVICE_ASSET_ROOT_DIR="/sdcard/Android/data/com.anki.cozmoengine/files/assets"
DEVICE_ASSET_DIR="$DEVICE_ASSET_ROOT_DIR/$SOURCE_HASH"

set +e
$ADB shell [ -f "$DEVICE_ASSET_DIR/cozmo_resources/allAssetHash.txt" ]
HAS_ASSETS=$?
set -e

if [ $HAS_ASSETS -eq 0 ]; then
    echo "assets available at $DEVICE_ASSET_DIR"
    exit 0
fi

# create target assets fir on device
$ADB shell mkdir -p $DEVICE_ASSET_DIR

# install new assets
echo "deploying assets"
pushd ${ASSETSDIR} > /dev/null 2>&1
$ADB push . ${DEVICE_ASSET_DIR}/
$ADB push ${ASSETSDIR}/cozmo_resources/allAssetHash.txt ${DEVICE_ASSET_ROOT_DIR}/current
popd > /dev/null 2>&1
echo "assets installed to $DEVICE_ASSET_DIR"

# reap old assets if there are more than 5 versions
DEVICE_DIR_LIST=($($ADB shell ls -1 $DEVICE_ASSET_ROOT_DIR))

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
