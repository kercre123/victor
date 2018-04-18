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

# TensorFlow uses a factory pattern that requires we forcibly include 
# everything from the the tensorflow-core library. Thus the --whole-archive flags

${CC} --std=c++11 -fPIE -mfloat-abi=softfp -mfpu=neon -pie \
  -DTENSORFLOW \
  --sysroot ${NDK_ROOT}/platforms/android-21/arch-arm \
  -I${NDK_ROOT}/sources/android/support/include \
  -I${NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.9/include \
  -I${NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi-v7a/include \
  -I${TENSORFLOW_PATH} \
  -I${TENSORFLOW_PATH}/bazel-tensorflow/external/eigen_archive \
  -I${TENSORFLOW_PATH}/bazel-genfiles \
  -I${TENSORFLOW_PATH}/bazel-tensorflow/external/protobuf_archive/src \
  -I${TENSORFLOW_PATH}/tensorflow/contrib/makefile/downloads/nsync/public \
  -I${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/android/sdk/native/jni/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/core/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/imgcodecs/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/imgproc/include \
  -I${VICTOR_REPO_PATH}/lib/util/source/anki \
  -I${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/include \
  standaloneForwardInference.cpp \
  objectDetector_tensorflow.cpp \
  ${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/source/jsoncpp.cpp \
  -L${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/lib/android_armeabi-v7a \
  -Wl,--whole-archive -ltensorflow-core -Wl,--no-whole-archive \
  ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/protobuf_android/armeabi-v7a/lib/libprotobuf.a -pthread \
  -L${NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi-v7a \
  -L${TENSORFLOW_PATH}/tensorflow/contrib/makefile/downloads/nsync/builds/armeabi-v7a.android.c++11 -lnsync \
  -L${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/android/sdk/native/libs/armeabi-v7a \
  -lopencv_imgproc -lopencv_core -lopencv_imgcodecs \
  -lgnustl_static -llog -lz -lm -ldl -latomic \
  -o ${OUTPUT_BINARY}

