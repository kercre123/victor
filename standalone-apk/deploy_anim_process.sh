#!/bin/sh

# Deploys and runs the Cozmo V2 animation process on a connected android device
#
# Must first build the cozmoAnim binary using configure.py build -p android -2 --features standalone.

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

# Check android version
DEVICE_API_VERSION=$($ADB shell getprop ro.build.version.sdk | tr -d '\r\n' | xargs printf "%d")
if [ "$DEVICE_API_VERSION" -lt "24" ]; then
    echo "Unsupported API version ($DEVICE_API_VERSION)"
    echo "Cozmo2 requires minimum Android N (7.0) - API 24"
    exit -1
fi

# Upload cozmoAnim process
ANIM_EXEC=../generated/android/out/Debug/cozmoAnim
if [ ! -f $ANIM_EXEC ]; then
    echo "ERROR: " $ANIM_EXEC " not found! Did you build it?"
    exit -2
fi
$ADB push $ANIM_EXEC /data/local/tmp

# Upload libs
CTE_PATH=../EXTERNALS/coretech_external
CTE_OPENCV_PATH=$CTE_PATH/build/opencv-android/OpenCV-android-sdk/sdk/native/libs/armeabi-v7a
declare -a LIBS=("../build/android/libs/armeabi-v7a/libc++_shared.so" \
                 "../generated/android/out/Debug/lib/libDAS.so" \
                 "$CTE_OPENCV_PATH/libopencv_calib3d.so" \
                 "$CTE_OPENCV_PATH/libopencv_core.so" \
                 "$CTE_OPENCV_PATH/libopencv_features2d.so" \
                 "$CTE_OPENCV_PATH/libopencv_flann.so" \
                 "$CTE_OPENCV_PATH/libopencv_ml.so" \
                 "$CTE_OPENCV_PATH/libopencv_highgui.so" \
                 "$CTE_OPENCV_PATH/libopencv_videoio.so" \
                 "$CTE_OPENCV_PATH/libopencv_imgcodecs.so" \
                 "$CTE_OPENCV_PATH/libopencv_imgproc.so" \
                 "$CTE_OPENCV_PATH/libtbb.so" \
                )
for lib in "${LIBS[@]}"
do
  # Check if lib is empty/spaces
  if [[ -z "${lib// }" ]]; then
    continue  
  fi

  # Check if file exists
  if [ ! -f $lib ]; then
    echo "ERROR: " $lib " not found!"
    continue
  fi

  echo "Uploading $lib"
  $ADB push $lib /data/local/tmp
done

# Shell into android device and execute robot process
$ADB shell -x "
export LD_LIBRARY_PATH=/data/local/tmp
cd /data/local/tmp
nice -n -20 ./cozmoAnim
"
