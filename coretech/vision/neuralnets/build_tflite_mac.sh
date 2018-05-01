#!/usr/bin/env bash

if [ "$#" -ne 2 ]; then
  echo Usage: $0 VICTOR_REPO_PATH OUTPUT_BINARY
  exit 1
fi

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

# Note: build TFLite static lib simply using "make -f tensorflow/contrib/lite/Makefile"

CC=g++

${CC} --std=c++11 \
  -DTFLITE \
  -g \
  -I${TENSORFLOW_PATH} \
  -I${TENSORFLOW_PATH}/bazel-tensorflow/external/eigen_archive \
  -I${TENSORFLOW_PATH}/bazel-genfiles \
  -I${TENSORFLOW_PATH}/tensorflow/contrib/lite/downloads/flatbuffers/include \
  -I${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/core/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/imgcodecs/include \
  -I${CORETECH_EXTERNAL_DIR}/opencv-3.4.0/modules/imgproc/include \
  -I${VICTOR_REPO_PATH}/lib/util/source/anki \
  -I${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/include \
  standaloneForwardInference.cpp \
  objectDetector_tflite.cpp \
  ${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/source/jsoncpp.cpp \
  ${TENSORFLOW_PATH}/tensorflow/contrib/lite/gen/lib/libtensorflow-lite.a \
  -L${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac/lib/Release -lopencv_imgproc -lopencv_imgcodecs -lopencv_core \
  -L${CORETECH_EXTERNAL_DIR}/build/opencv-3.4.0/mac/3rdparty/lib/Release -lippicv -lippiw -llibpng -llibjpeg -llibtiff -lzlib -littnotify -llibjasper -lIlmImf \
  -F${FRAMEWORK_PATH} -framework OpenCL -framework Accelerate \
  -lstdc++ -lm \
  -o ${OUTPUT_BINARY}
