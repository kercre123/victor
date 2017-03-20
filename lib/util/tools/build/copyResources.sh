#!/bin/bash
# Copyright (c) 2014 Anki, Inc.
# All rights reserved.

# Exit for non-zero error
set -e

# Exit if a needed environment variable is not set
set -u

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
source "${SCRIPT_PATH}/build_env.sh"

profile_begin "" "[$@]"

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`
cd $TOPLEVEL

function usage() {
    echo "$0 [-c] [-h] [-t platform] [-d dist_root] "
    echo "-t platform     : target platform. Can be 'android', 'ios', 'unity'"
    echo "-d dist_root    : Distribution dir"
    echo "-a asset_root   : Asset repo location"
    echo "-b bs_root      : basestation repo location"
    echo "-u              : copy data for unit tests"
    echo "-h              : help. Print out this info"
    echo ""
    echo "assembles all resources into final deployment location"
}


DIST_ROOT=
ASSET_ROOT=
BS_ROOT=
UNIT_TEST=

while getopts ":t:hd:a:b:u" opt; do
    case $opt in
        h)
            usage
            exit 0
            ;;
        t)
            TARGET=$OPTARG
            ;;
        d)
            DIST_ROOT=$OPTARG
            ;;
        a)
            ASSET_ROOT=$OPTARG
            ;;
        b)
            BS_ROOT=$OPTARG
            ;;
        u)
            UNIT_TEST=1
            ;;
        \?)
            echo "Invalid option: -$OPTARG" >&2
            usage
            exit 1
            ;;
        :)
            echo "Option -$OPTARG requires an argument." >&2
            usage
            exit 1
            ;;
    esac
done


if [ ! $DIST_ROOT ]; then
    echo "dist root is empty" >&2
    usage
    exit 1
fi

if [ ! $ASSET_ROOT ]; then
    echo "asset root is empty" >&2
    usage
    exit 1
fi

if [ ! $BS_ROOT ]; then
    echo "basestation root is empty" >&2
    usage
    exit 1
fi


pushd $ASSET_ROOT > /dev/null
# execute asset build
./build.sh -t $TARGET
popd > /dev/null

# copy sound assets into the distribution folder if they exist
if [ -d $ASSET_ROOT/build/$TARGET/dist/sound ]; then
  rsync -r -t --delete $ASSET_ROOT/build/$TARGET/dist/sound $DIST_ROOT/
fi

# make DIST_ROOT if it doesn't exist
mkdir -p "${DIST_ROOT}"

#copy game config assets into the distribution folder
rsync -r -t --delete $ASSET_ROOT/build/$TARGET/dist/gameData/* $DIST_ROOT/config

#copy basestation config assets into the distribution folder
if [ ! -z  $UNIT_TEST ]; then
    rsync -r -t --delete \
    --exclude 'sources' \
    --exclude '*.xlsx' \
    --exclude 'config/mapFiles/test/' \
    --exclude 'config/roadPieceDefinitionFiles/test/' \
    --exclude 'config/scripts/test/' \
    --exclude 'config/tests/' \
    $BS_ROOT/resources/config/* $DIST_ROOT/config
else
    rsync -r -t --delete \
    --exclude 'sources' \
    --exclude '*.xlsx' \
    $BS_ROOT/resources/config/* $DIST_ROOT/config
fi

# copy firmware into the distribution folder
rsync -r -t --delete \
    --exclude '*.h' \
    --exclude '*.safe' \
    $BS_ROOT/resources/firmware/ $DIST_ROOT/firmware/

#merge string maps together
$BS_ROOT/tools/build/mergeStringMaps.sh \
$ASSET_ROOT/build/$TARGET/dist/gameData/stringMap.json \
$BS_ROOT/resources/config/stringMap.json \
$DIST_ROOT/config/stringMap.json

profile_end ""
