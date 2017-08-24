#!/bin/bash

set -e

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))            
_SCRIPT_NAME=`basename ${0}`                                                                        

GIT=`which git`                                                                                     
if [ -z $GIT ];then                                                                                 
  echo git not found                                                                                
  exit 1                                                                                            
fi                                                                                                  
TOPLEVEL=`$GIT rev-parse --show-toplevel`

: ${ANKI_BUILD_TOOLS:="${TOPLEVEL}/tools/build/tools"}
: ${CMAKE_EXE="${ANDROID_HOME}/cmake/3.6.3155560/bin/cmake"}
: ${BUILD_DIR=_build}
: ${GEN_SRC_DIR:=generated/cmake}


mkdir -p $BUILD_DIR
mkdir -p $GEN_SRC_DIR

# generate file lists
${ANKI_BUILD_TOOLS}/metabuild/metabuild.py -v -o ${GEN_SRC_DIR} \
        source/anki/util/BUILD.in

pushd $BUILD_DIR > /dev/null 2>&1

$CMAKE_EXE .. \
    -DCMAKE_TOOLCHAIN_FILE=cmake/android.toolchain.r14-patched.cmake \
    -DANDROID_NDK=${ANDROID_NDK_ROOT} \
    -DANDROID_TOOLCHAIN_NAME=clang \
    -DANDROID_API=android-24 \
    -DANDROID_ABI="armeabi-v7a with NEON" \
    -DANDROID_STL=c++_shared \
    -DANDROID_CPP_FEATURES="rtti exceptions" \
    -DCMAKE_VERBOSE_MAKEFILE=ON

$CMAKE_EXE --build .

popd > /dev/null 2>&1 # $BUILD_DIR

