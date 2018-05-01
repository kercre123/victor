#!/usr/bin/env bash

if [ "$#" -ne 3 ]; then
  echo Usage: $0 VICTOR_REPO_PATH NDK_ROOT OUTPUT_BINARY
  exit 1
fi

VICTOR_REPO_PATH=$1
NDK_ROOT=$2
OUTPUT_BINARY=$3
CORETECH_EXTERNAL_DIR=${VICTOR_REPO_PATH}/EXTERNALS/coretech_external
TENSORFLOW_PATH=${CORETECH_EXTERNAL_DIR}/tensorflow

echo "--Using Android NDK at ${NDK_ROOT}"
echo "--Looking for tensorflow at ${TENSORFLOW_PATH}"

if [ ! -d ${TENSORFLOW_PATH} ] 
then
  echo "TensorFlow (or symlink to it) should be in ${TENSORFLOW_PATH}"
  exit 1
fi

CC=${NDK_ROOT}/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-g++

${CC} --std=c++11 -fPIE -mfloat-abi=softfp -mfpu=neon -pie \
  -DTFLITE \
  --sysroot ${NDK_ROOT}/platforms/android-21/arch-arm \
  -I${NDK_ROOT}/sources/android/support/include \
  -I${NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.9/include \
  -I${NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi-v7a/include \
  -I${TENSORFLOW_PATH} \
  -I${TENSORFLOW_PATH}/bazel-tensorflow/external/eigen_archive \
  -I${TENSORFLOW_PATH}/tensorflow/contrib/lite/downloads/flatbuffers/include \
  -I${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/android/sdk/native/jni/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/core/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/imgcodecs/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/imgproc/include \
  -I${VICTOR_REPO_PATH}/lib/util/source/anki \
  -I${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/include \
  standaloneForwardInference.cpp \
  objectDetector_tflite.cpp \
  ${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/source/jsoncpp.cpp \
  ${TENSORFLOW_PATH}/tensorflow/contrib/lite/gen_ANDROID/lib/libtensorflow-lite.a \
  -L${NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi-v7a \
  -L${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/android/sdk/native/libs/armeabi-v7a \
  -lopencv_imgproc -lopencv_core -lopencv_imgcodecs \
  -lgnustl_static -llog -lz -lm -ldl -latomic \
  -o ${OUTPUT_BINARY}

