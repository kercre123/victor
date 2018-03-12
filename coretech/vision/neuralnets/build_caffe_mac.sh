#!/usr/bin/env bash

if [ "$#" -ne 2 ]; then
  echo Usage: $0 VICTOR_REPO_PATH OUTPUT_BINARY
  exit 1
fi

VICTOR_REPO_PATH=$1
OUTPUT_BINARY=$2
CORETECH_EXTERNAL_DIR=${VICTOR_REPO_PATH}/EXTERNALS/coretech_external
CAFFE2_PATH=${CORETECH_EXTERNAL_DIR}/caffe2
FRAMEWORK_PATH=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.13.sdk/System/Library/Frameworks

echo "--Looking for frameworks at ${FRAMEWORK_PATH}"
echo "--Looking for caffe2 at ${CAFFE2_PATH}"

if [ ! -d ${CAFFE2_PATH} ] 
then
  echo "Caffe2 (or symlink to it) should be in ${CAFFE2_PATH}"
  exit 1
fi


CC=g++

${CC} --std=c++11 -g \
  -DCAFFE2 \
  -I${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/core/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/imgcodecs/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/imgproc/include \
  -I${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/include \
  standaloneTensorFlowInference.cpp \
  objectDetector_caffe2.cpp \
  ${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/source/jsoncpp.cpp \
  -F${FRAMEWORK_PATH} -framework OpenCL -framework Accelerate \
  -lstdc++ -lm \
  -L${CAFFE2_PATH}/build/lib -lpthreadpool -lprotobuf -lglog -lgflags -lnnpack -lcaffe2d \
  -L${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac/lib/Release  -lcpuinfo -lopencv_imgproc -lopencv_imgcodecs -lopencv_core \
  -L${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac/3rdparty/lib/Release -lippicv -lippiw -llibpng -llibjpeg -llibtiff -lzlib -littnotify -llibjasper -lIlmImf \
  -o ${OUTPUT_BINARY}

  #-Wl,-force_load ${CAFFE2_PATH}/build/lib/libcaffe2.a \
