#!/bin/bash

set -e
set -u

CLAD="${PYTHON:=python} ${PYTHONFLAGS:=} ${EMITTER:=../../CPP_emitter.py}"
CLADSRC=../src
OUTPUT_DIR=${OUTPUT_DIR:-./build/simple}
SUPPORTDIR=../../../support/cpp

for file in $CLADSRC/*.clad $CLADSRC/aligned/*.clad; do
    OUTPUT_DIR_PARAM=$(dirname $OUTPUT_DIR/${file#$CLADSRC/};)
    mkdir -p ${OUTPUT_DIR_PARAM}
    $CLAD -o ${OUTPUT_DIR_PARAM} -C $(dirname $file) $(basename $file);
done

cp -f cpptest.cpp $OUTPUT_DIR

clang++ -Wall -Wextra -std=c++11 -stdlib=libc++ \
    -I$SUPPORTDIR/include -g -os -o $OUTPUT_DIR/cpptest.out \
    $OUTPUT_DIR/cpptest.cpp \
    $OUTPUT_DIR/SimpleTest.cpp \
    $OUTPUT_DIR/aligned/AutoUnionTest.cpp \
    $OUTPUT_DIR/Foo.cpp \
    $OUTPUT_DIR/Bar.cpp \
    $OUTPUT_DIR/aligned/AnkiEnum.cpp \
    $SUPPORTDIR/source/SafeMessageBuffer.cpp
$OUTPUT_DIR/cpptest.out

