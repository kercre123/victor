#!/usr/bin/env bash

CORETECH_PATH=/Users/andrew/Code/victor/coretech
TENSORFLOW_PATH=/Users/andrew/Code/tensorflow
NDK_ROOT=/Users/andrew/.anki/android/ndk-repository/android-ndk-r14b
VICTOR_REPO_PATH=/Users/andrew/Code/victor

CC=${NDK_ROOT}/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-g++

# TensorFlow uses a factory pattern that requires we forcibly include 
# everything from the the tensorflow-core library. Thus the --whole-archive flags

${CC} --std=c++11 \
  --sysroot ${NDK_ROOT}/platforms/android-21/arch-arm \
  -I${NDK_ROOT}/sources/android/support/include \
  -I${NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.9/include \
  -I${NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi-v7a/include \
  -I${TENSORFLOW_PATH}/lib/python2.7/site-packages/tensorflow/include \
  -I${TENSORFLOW_PATH}/tensorflow/contrib/makefile/downloads/nsync/public \
  -I${VICTOR_REPO_PATH}/EXTERNALS/coretech_external/build/opencv-3.4.0/android/sdk/native/jni/include \
  -I${VICTOR_REPO_PATH}/EXTERNALS/coretech_external/opencv-3.4.0/modules/core/include \
  -I${VICTOR_REPO_PATH}/EXTERNALS/coretech_external/opencv-3.4.0/modules/imgcodecs/include \
  -I${VICTOR_REPO_PATH}/EXTERNALS/coretech_external/opencv-3.4.0/modules/imgproc/include \
  -I${VICTOR_REPO_PATH}/lib/util/source/anki \
  -I${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/include \
  standaloneTensorFlowInference.cpp \
  ${VICTOR_REPO_PATH}/tools/message-buffers/support/cpp/source/jsoncpp.cpp \
  -L${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/lib/android_armeabi-v7a -Wl,--whole-archive -ltensorflow-core -Wl,--no-whole-archive \
  -L${NDK_ROOT}/sources/cxx-stl/gnu-libstdc++/4.9/libs/armeabi-v7a \
  -L${TENSORFLOW_PATH}/tensorflow/contrib/makefile/downloads/nsync/builds/armeabi-v7a.android.c++11 -lnsync \
  -L${VICTOR_REPO_PATH}/EXTERNALS/coretech_external/build/opencv-3.4.0/android/sdk/native/libs/armeabi-v7a -lopencv_imgproc -lopencv_core -lopencv_imgcodecs \
  -lgnustl_static -llog -lz -lm -ldl -latomic \
  -L${TENSORFLOW_PATH}/tensorflow/contrib/makefile/gen/protobuf_android/armeabi-v7a/lib -lprotobuf -pthread \
  -o standaloneTensorFlowInference_android