#!/usr/bin/env bash

if [ "$#" -ne 3 ]; then
  echo Usage: $0 VICTOR_REPO_PATH NDK_ROOT OUTPUT_BINARY
  exit 1
fi

VICTOR_REPO_PATH=$1
NDK_ROOT=$2
OUTPUT_BINARY=$3
CORETECH_EXTERNAL_DIR=${VICTOR_REPO_PATH}/EXTERNALS/coretech_external

echo "--Using Android NDK at ${NDK_ROOT}"

CC=${NDK_ROOT}/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-g++

${CC} --std=c++11 -fPIE -mfloat-abi=softfp -mfpu=neon -pie \
  -DOPENCV_DNN \
  --sysroot ${NDK_ROOT}/platforms/android-21/arch-arm \
  -I${NDK_ROOT}/sources/android/support/include \
  -I${NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.9/include \
  -I${NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi-v7a/include \
  -I${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/core/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/imgcodecs/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/imgproc/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/dnn/include \
  -I${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/android/sdk/native/jni/include \
  -I${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/include \
  standaloneForwardInference.cpp \
  objectDetector_opencvdnn.cpp \
  ${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/source/jsoncpp.cpp \
  -L${NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi-v7a \
  -lgnustl_static -llog -lz -lm -ldl -latomic \
  -L${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/android/sdk/native/libs/armeabi-v7a \
  -lopencv_imgproc -lopencv_imgcodecs -lopencv_core -lopencv_dnn \
  -L${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/android/3rdparty/lib/armeabi-v7a -llibpng -llibjpeg -llibtiff -llibprotobuf \
  -o ${OUTPUT_BINARY}
