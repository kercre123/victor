#!/bin/sh

# Deploys and runs the Cozmo V2 robot process on a connected android device
#
# Must first build the cozmoRobot2 binary using configure.py build -p android.
# Optionally, you can call configure.py run -l -p android --features standalone to deploy 
# the standalone engine. Once it's running, you can call this script since the same
# command also builds the cozmoRobot2 target.
# Then you'll want to call ./tools/sdk_devonly/start_sim.py -r -v2 to connect the
# engine and robot processes.

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

$ADB root

# Shell into android device and execute robot process
$ADB shell -x "
echo 1 > /sys/kernel/debug/regulator/8916_l10/enable
export LD_LIBRARY_PATH=/data/local/tmp
cd /data/local/tmp
nice -n -20 ./cozmoRobot2
"
