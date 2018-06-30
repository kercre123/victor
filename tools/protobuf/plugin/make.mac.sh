#!/bin/bash

GIT_ROOT=`git rev-parse --show-toplevel`
PROTOBUF_LOC="${GIT_ROOT}/EXTERNALS/coretech_external/build/protobuf/mac/"
INCLUDES="${PROTOBUF_LOC}/include/"
LIBS="${PROTOBUF_LOC}/lib/"
SRCS=`find . -type f -iname "*.cpp"`
OUTPUT="protocCppPlugin"

clang++                               \
    --std=c++14                       \
    ${LIBS}/libprotoc.a               \
    ${LIBS}/libprotobuf.a             \
    -I.  -I${INCLUDES}                \
    ${SRCS}                           \
    -o ${OUTPUT}
    