#!/bin/bash

set -e
set -u

if [ ! -d standalone-apk ]; then
    echo "Must be run from top level directory"
    exit 1
fi

# Copy the assets
RESDIR=`pwd`/standalone-apk/resources
ASSETSDIR=$RESDIR/assets
rm -rf $ASSETSDIR
mkdir -p $ASSETSDIR
rm -f $RESDIR/assets.zip
pushd unity/Cozmo/Assets/StreamingAssets
zip $RESDIR/assets.zip `cat resources.txt` cozmo_resources/sound/AudioAssets.zip
popd
unzip $RESDIR/assets.zip -d $ASSETSDIR
if [ ! -e $ASSETSDIR/resources.txt ]; then
    cp -pv unity/Cozmo/Assets/StreamingAssets/resources.txt $ASSETSDIR/
fi
rm $RESDIR/assets.zip


