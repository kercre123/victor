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
: ${INSTALL_ROOT:="/anki"}

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

# Check that the assets directory exists
if [ ! -d "$ASSETSDIR" ]; then
  echo "Assets directory ${ASSETSDIR} does not exist!"
  exit 1
fi

DEVICE_RSYNC_BIN_DIR="/tmp"
DEVICE_RSYNC_CONF_DIR="/data/rsync"

# increment the following value if the contents of rsyncd.conf change
RSYNCD_CONF_VERSION=1

DEVICE_ASSET_REL_DIR="files"
DEVICE_ASSET_ROOT_DIR="${INSTALL_ROOT}/data/files/assets"
DEVICE_ASSET_DIR="$DEVICE_ASSET_ROOT_DIR/$DEVICE_ASSET_REL_DIR"
RSYNC_BIN_DIR=(${TOPLEVEL}/tools/rsync)

# source adb env & helper functions
source android_env.sh

# get device IP Address
DEVICE_IP_ADDRESS=`$ADB shell ip addr show wlan0 | grep "inet\s" | awk '{print $2}' | awk -F'/' '{print $1}'`
if [ -z $DEVICE_IP_ADDRESS ]; then
  DEVICE_IP_ADDRESS=`$ADB shell ip addr show lo | grep "inet\s" | awk '{print $2}' | awk -F'/' '{print $1}'`
  if [ -z $DEVICE_IP_ADDRESS ]; then
    echo "no valid android device found"
    exit 1
  fi

  DEVICE_IP_ADDRESS="$DEVICE_IP_ADDRESS:6010"
  $ADB forward tcp:6010 tcp:1873
else
  DEVICE_IP_ADDRESS="$DEVICE_IP_ADDRESS:1873"
fi

# delete all old bundles from asset folder if REMOVE_ALL_ASSETS=1
if [ $REMOVE_ALL_ASSETS -eq 1 ]; then
  $ADB shell rm -rf ${DEVICE_ASSET_ROOT_DIR}/*
# blow away current assets folder to force re-push if FORCE_PUSH_ASSETS=1
elif [ $FORCE_PUSH_ASSETS -eq 1 ]; then
  $ADB shell rm -rf ${DEVICE_ASSET_DIR}
fi

# Install new assets
pushd ${ASSETSDIR} > /dev/null 2>&1

# Make sure we have the directories we expect
$ADB shell mkdir -p ${DEVICE_RSYNC_BIN_DIR}
$ADB shell mkdir -p ${DEVICE_RSYNC_CONF_DIR}
$ADB shell mkdir -p ${DEVICE_ASSET_DIR}

# install rsync binary and config if needed
set +e
adb_shell "[ -f "$DEVICE_RSYNC_BIN_DIR/rsync.bin" ]"
if [ $? -ne 0 ]; then
  echo "loading rsync to device"
  $ADB push ${RSYNC_BIN_DIR}/rsync.bin ${DEVICE_RSYNC_BIN_DIR}
fi

RSYNCD_CONF="rsyncd-v${RSYNCD_CONF_VERSION}.conf"

adb_shell "[ -f "$DEVICE_RSYNC_CONF_DIR/$RSYNCD_CONF" ]"
if [ $? -ne 0 ]; then
  echo "loading rsync config to device"
  $ADB push ${RSYNC_BIN_DIR}/rsyncd.conf ${DEVICE_RSYNC_CONF_DIR}/$RSYNCD_CONF
fi
set -e

echo "deploying assets: ${ASSETSDIR}"

# startup rsync daemon
$ADB shell "${DEVICE_RSYNC_BIN_DIR}/rsync.bin --daemon \
    --config=${DEVICE_RSYNC_CONF_DIR}/${RSYNCD_CONF}"

# sync files
rsync -rcP --stats --delete ./ rsync://${DEVICE_IP_ADDRESS}/assets/${DEVICE_ASSET_REL_DIR}/

# record the asset directory for the animation process
$ADB shell "echo '${DEVICE_ASSET_REL_DIR}' > '${DEVICE_ASSET_ROOT_DIR}/current'"

popd > /dev/null 2>&1
echo "assets installed to $DEVICE_ASSET_DIR"
