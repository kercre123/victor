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
cd "${0%/*}"

# List connect devices
adb devices

# Grab the IDs of all the connected devices / emulators
IDS=($(adb devices | sed '1,1d' | sed '$d' | cut -f 1 | sort))
NUMIDS=${#IDS[@]}

# Check for number of connected devices / emulators
if [[ 0 -eq "$NUMIDS" ]]; then
    # No IDs, exit
    echo "No android devices detected - nothing to do."
    exit -1;
fi

# Upload cozmoRobot2 process
ROBOT_EXEC=../generated/android/out/Debug/cozmoRobot2
if [ ! -f $ROBOT_EXEC ]; then
    echo "ERROR: " $ROBOT_EXEC " not found! Did you build it?"
    exit -2
fi
adb push $ROBOT_EXEC /data/local/tmp

# Upload C++ lib
CPP_LIB=../build/android/libs/armeabi-v7a/libc++_shared.so
if [ ! -f $CPP_LIB ]; then
    echo "ERROR: " $CPP_LIB " not found!"
    exit -2
fi
adb push $CPP_LIB /data/local/tmp

# Shell into android device and execute robot process
adb shell "
export LD_LIBRARY_PATH=/data/local/tmp
cd /data/local/tmp
nice -n -20 ./cozmoRobot2
"