#!/usr/bin/env bash

CORETECH_PATH=/Users/andrew/Code/victor/coretech
TENSORFLOW_PATH=/Users/andrew/Code/tensorflow
VICTOR_REPO_PATH=/Users/andrew/Code/victor
FRAMEWORK_PATH=/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.13.sdk/System/Library/Frameworks

CC=g++

${CC} --std=c++11 \
  -I${TENSORFLOW_PATH}/lib/python2.7/site-packages/tensorflow/include \
  -I${TENSORFLOW_PATH}/tensorflow/contrib/makefile/downloads/nsync/public \
  -I${VICTOR_REPO_PATH}/EXTERNALS/coretech_external/build/opencv-3.4.0/mac \
  -I${VICTOR_REPO_PATH}/EXTERNALS/coretech_external/opencv-3.4.0/modules/core/include \
  -I${VICTOR_REPO_PATH}/EXTERNALS/coretech_external/opencv-3.4.0/modules/imgcodecs/include \
  -I${VICTOR_REPO_PATH}/EXTERNALS/coretech_external/opencv-3.4.0/modules/imgproc/include \
  -I${VICTOR_REPO_PATH}/lib/util/source/anki \
  -I${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/include \
  standaloneTensorFlowInference.cpp \
  ${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/source/jsoncpp.cpp \
  -Wl,-force_load ${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/lib/libtensorflow-core.a \
  -L${TENSORFLOW_PATH}/tensorflow/contrib/makefile/downloads/nsync/builds/default.macos.c++11 -lnsync \
  -L${VICTOR_REPO_PATH}/EXTERNALS/coretech_external/build/opencv-3.4.0/mac/lib/Release   -lopencv_imgproc -lopencv_imgcodecs -lopencv_core \
  -L${VICTOR_REPO_PATH}/EXTERNALS/coretech_external/build/opencv-3.4.0/mac/3rdparty/lib/Release -lippicv -lippiw -llibpng -llibjpeg -llibtiff -lzlib -littnotify \
  -F${FRAMEWORK_PATH} -framework OpenCL -framework Accelerate \
  -lstdc++ -lm \
  -L${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/protobuf/lib -lprotobuf -pthread \
  -o standaloneTensorFlowInference_mac