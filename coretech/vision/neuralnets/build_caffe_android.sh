#!/usr/bin/env bash

if [ "$#" -ne 3 ]; then
  echo Usage: $0 VICTOR_REPO_PATH NDK_ROOT OUTPUT_BINARY
  exit 1
fi

VICTOR_REPO_PATH=$1
NDK_ROOT=$2
OUTPUT_BINARY=$3
CORETECH_EXTERNAL_DIR=${VICTOR_REPO_PATH}/EXTERNALS/coretech_external
CAFFE2_PATH=${CORETECH_EXTERNAL_DIR}/caffe2

echo "--Using Android NDK at ${NDK_ROOT}"
echo "--Looking for caffe2 at ${CAFFE2_PATH}"

if [ ! -d ${CAFFE2_PATH} ] 
then
  echo "Caffe2 (or symlink to it) should be in ${CAFFE2_PATH}"
  exit 1
fi

#STL=llvm-libc++
STL=gnu-libstdc++/4.9

CC=${NDK_ROOT}/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-g++

${CC} --std=c++11 -fPIE -mfloat-abi=softfp -mfpu=neon -pie \
  --sysroot ${NDK_ROOT}/platforms/android-21/arch-arm \
  -DCAFFE2 \
  -I${CAFFE2_PATH} \
  -I${CAFFE2_PATH}/build_android \
  -I${CAFFE2_PATH}/third_party/protobuf/src \
  -I${NDK_ROOT}/sources/android/support/include \
  -I${NDK_ROOT}/sources/cxx-stl/${STL}/include \
  -I${NDK_ROOT}/sources/cxx-stl/${STL}/libs/armeabi-v7a/include \
  -I${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/core/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/imgcodecs/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/imgproc/include \
  -I${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/include \
  standaloneForwardInference.cpp \
  objectDetector_caffe2.cpp \
  ${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/source/jsoncpp.cpp \
  -L${CAFFE2_PATH}/build_android/lib \
  -Wl,--whole-archive ${CAFFE2_PATH}/build_android/lib/libcaffe2.a -Wl,--no-whole-archive \
  -lpthreadpool -lprotobuf -lnnpack -lcpuinfo -lcpufeatures \
  -L${NDK_ROOT}/sources/cxx-stl/${STL}/libs/armeabi-v7a \
  -lgnustl_static -llog -lz -lm -ldl -latomic -pthread \
  -L${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/android/sdk/native/libs/armeabi-v7a \
  -lopencv_imgproc -lopencv_imgcodecs -lopencv_core \
  -o ${OUTPUT_BINARY}
