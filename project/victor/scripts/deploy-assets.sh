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
: ${INSTALL_ROOT:="/data/data/com.anki.cozmoengine"}

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

DEVICE_RSYNC_BIN_DIR="/data/local/tmp"
DEVICE_RSYNC_CONF_DIR="/data/rsync"

DEVICE_ASSET_REL_DIR="files"
DEVICE_ASSET_ROOT_DIR="${INSTALL_ROOT}/files/assets"
DEVICE_ASSET_DIR="$DEVICE_ASSET_ROOT_DIR/$DEVICE_ASSET_REL_DIR"
RSYNC_BIN_DIR=(${TOPLEVEL}/tools/rsync)

# source adb env & helper functions
source android_env.sh

# get device IP Address
DEVICE_IP_ADDRESS=`($ADB devices) | grep -o "[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}"`

# delete all old bundles from asset folder if REMOVE_ALL_ASSETS=1
if [ $REMOVE_ALL_ASSETS -eq 1 ]; then
  $ADB shell rm -rf ${DEVICE_ASSET_ROOT_DIR}/*
# blow away current assets folder to force re-push if FORCE_PUSH_ASSETS=1
elif [ $FORCE_PUSH_ASSETS -eq 1 ]; then
  $ADB shell rm -rf ${DEVICE_ASSET_DIR}
fi

# Install new assets
pushd ${ASSETSDIR} > /dev/null 2>&1

# install rsync binary and config if needed
set +e
$ADB shell [ -f "$DEVICE_RSYNC_BIN_DIR/rsync.bin" ]
HAS_RSYNC=$?
set -e

if [ $FORCE_PUSH_ASSETS -eq 1 ] || [ $REMOVE_ALL_ASSETS -eq 1 ] || [ $HAS_RSYNC -ne 0 ]; then

  echo "loading rsync to device"
  $ADB shell mkdir -p ${DEVICE_RSYNC_BIN_DIR}
  $ADB shell mkdir -p ${DEVICE_RSYNC_CONF_DIR}

  $ADB push ${RSYNC_BIN_DIR}/rsync.bin ${DEVICE_RSYNC_BIN_DIR}
  $ADB push ${RSYNC_BIN_DIR}/rsyncd.conf ${DEVICE_RSYNC_CONF_DIR}
fi

echo "deploying assets"

# startup rsync daemon
$ADB shell "${DEVICE_RSYNC_BIN_DIR}/rsync.bin --daemon --config=${DEVICE_RSYNC_CONF_DIR}/rsyncd.conf &"

# sync files
rsync -rP --stats --delete . rsync://${DEVICE_IP_ADDRESS}/files/${DEVICE_ASSET_REL_DIR}

# record the asset directory for the animation process
$ADB shell "echo '${DEVICE_ASSET_REL_DIR}' > '${DEVICE_ASSET_ROOT_DIR}/current'"

popd > /dev/null 2>&1
echo "assets installed to $DEVICE_ASSET_DIR"
