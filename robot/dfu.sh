#!/bin/sh

# Updates the Victor robot firmware on a connected android device
#
# Must first build the syscon firmware using
#  > cd robot
#  > ./vmake.sh
#

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

ADB=adb
if [ -e $TOPLEVEL/local.properties ]; then
    ANDROID_HOME=`egrep sdk.dir $TOPLEVEL/local.properties | awk -F= '{print $2;}'`
    ADB=$ANDROID_HOME/platform-tools/adb
    if [ ! -x $ADB ]; then
        ADB=adb
    fi
fi

# List connect devices
$ADB devices

# Grab the IDs of all the connected devices / emulators
IDS=($($ADB devices | sed '1,1d' | sed '$d' | cut -f 1 | sort))
NUMIDS=${#IDS[@]}

# Check for number of connected devices / emulators
if [[ 0 -eq "$NUMIDS" ]]; then
    # No IDs, exit
    echo "No android devices detected - nothing to do."
    exit -1;
fi

ISROOT=`$ADB root`
if [[ ${#ISROOT} -lt 31 ]]; then
    echo "Running userinit"
    $ADB shell -x "./data/local/tmp/userinit.sh"
fi

cd hal/dfu
make
#make the remote schtuff?
$ADB push dfu /data/local/tmp
cd ../..
$ADB push syscon/build/syscon.bin /data/local/tmp

# Shell into android device and execute dfu process
$ADB shell -x "
export LD_LIBRARY_PATH=/data/local/tmp
cd /data/local/tmp
./dfu syscon.bin

"
