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

DEVICE_INSTALL_PATH="/data/data/com.anki.cozmoengine/lib"
adb shell mkdir -p "${DEVICE_INSTALL_PATH}"

# deploy opencv
function deploy-opencv()
{
  adb shell mkdir -p "${DEVICE_INSTALL_PATH}/opencv"
  OPENCV_LIB_DIR="${TOPLEVEL}/EXTERNALS/coretech_external/build/opencv-android/OpenCV-android-sdk/sdk/native/libs/armeabi-v7a"
  for LIB in ${OPENCV_LIB_DIR}/*.so; do
    adb push "${LIB}" "${DEVICE_INSTALL_PATH}/opencv"
  done
}

deploy-opencv

for SO in ${TOPLEVEL}/_build/android/Debug/lib/*.so; do
    adb push "${SO}" "${DEVICE_INSTALL_PATH}"
done
