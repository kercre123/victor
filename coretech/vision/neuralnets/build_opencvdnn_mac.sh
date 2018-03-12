#!/usr/bin/env bash

if [ "$#" -ne 2 ]; then
  echo Usage: $0 VICTOR_REPO_PATH OUTPUT_BINARY
  exit 1
fi

VICTOR_REPO_PATH=$1
OUTPUT_BINARY=$2
CORETECH_EXTERNAL_DIR=${VICTOR_REPO_PATH}/EXTERNALS/coretech_external
FRAMEWORK_PATH=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.13.sdk/System/Library/Frameworks

echo "--Looking for frameworks at ${FRAMEWORK_PATH}"


CC=g++

${CC} --std=c++11 \
  -DOPENCV_DNN \
  -I${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/core/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/imgcodecs/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/imgproc/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/dnn/include \
  -I${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/include \
  standaloneTensorFlowInference.cpp \
  objectDetector_opencvdnn.cpp \
  ${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/source/jsoncpp.cpp \
  -F${FRAMEWORK_PATH} -framework OpenCL -framework Accelerate \
  -lstdc++ -lm \
  -L${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac/lib/Release   -lopencv_imgproc -lopencv_imgcodecs -lopencv_core -lopencv_dnn \
  -L${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac/3rdparty/lib/Release -lippicv -lippiw -llibpng -llibjpeg -llibtiff -lzlib -littnotify -llibjasper -lIlmImf -llibprotobuf \
  -o ${OUTPUT_BINARY}
