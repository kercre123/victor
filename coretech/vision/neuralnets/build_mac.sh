#!/usr/bin/env bash

VICTOR_REPO_PATH=$1
OUTPUT_BINARY=$2
CORETECH_EXTERNAL_DIR=${VICTOR_REPO_PATH}/EXTERNALS/coretech_external
TENSORFLOW_PATH=${CORETECH_EXTERNAL_DIR}/tensorflow
FRAMEWORK_PATH=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.13.sdk/System/Library/Frameworks

echo "--Looking for frameworks at ${FRAMEWORK_PATH}"
echo "--Looking for tensorflow at ${TENSORFLOW_PATH}"

if [ ! -d ${TENSORFLOW_PATH} ] 
then
  echo "TensorFlow (or symlink to it) should be in ${TENSORFLOW_PATH}"
  exit 1
fi


CC=g++

${CC} --std=c++11 \
  -I${TENSORFLOW_PATH}/lib/python2.7/site-packages/tensorflow/include \
  -I${TENSORFLOW_PATH}/tensorflow/contrib/makefile/downloads/nsync/public \
  -I${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/core/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/imgcodecs/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/imgproc/include \
  -I${VICTOR_REPO_PATH}/lib/util/source/anki \
  -I${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/include \
  standaloneTensorFlowInference.cpp \
  objectDetector_tensorflow.cpp \
  ${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/source/jsoncpp.cpp \
  -Wl,-force_load ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/lib/libtensorflow-core.a \
  -L${TENSORFLOW_PATH}/tensorflow/contrib/makefile/downloads/nsync/builds/default.macos.c++11 -lnsync \
  -L${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac/lib/Release   -lopencv_imgproc -lopencv_imgcodecs -lopencv_core \
  -L${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac/3rdparty/lib/Release -lippicv -lippiw -llibpng -llibjpeg -llibtiff -lzlib -littnotify \
  -F${FRAMEWORK_PATH} -framework OpenCL -framework Accelerate \
  -lstdc++ -lm \
  -L${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/protobuf/lib -lprotobuf -pthread \
  -o ${OUTPUT_BINARY}
