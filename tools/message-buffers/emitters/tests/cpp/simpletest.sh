#!/bin/bash

set -eu

CLAD="${PYTHON:=python} ${PYTHONFLAGS:=} ${EMITTER:=../../CPP_emitter.py}"
CLADSRC=../src
OUTPUT_DIR=${OUTPUT_DIR:-./build/simple}
SUPPORTDIR=../../../support/cpp
TEST_SUPPORTDIR=../support

for file in $CLADSRC/Foo.clad $CLADSRC/Bar.clad $CLADSRC/SimpleTest.clad \
                              $CLADSRC/ExplicitUnion.clad \
                              $CLADSRC/ExplicitAutoUnion.clad \
                              $CLADSRC/UnionOfUnion.clad \
                              $CLADSRC/aligned/AutoUnionTest.clad \
                              $CLADSRC/aligned/AnkiEnum.clad \
                              $CLADSRC/DefaultValues.clad; do
    OUTPUT_DIR_PARAM=$(dirname $OUTPUT_DIR/${file#$CLADSRC/};)
    mkdir -p ${OUTPUT_DIR_PARAM}
    $CLAD --output-union-helper-constructors -o ${OUTPUT_DIR_PARAM} -C $(dirname $file) $(basename $file);
done

cp -f cpptest.cpp $OUTPUT_DIR

clang++ -Wall -Wextra -std=c++11 -stdlib=libc++ \
    -I$TEST_SUPPORTDIR -I$SUPPORTDIR/include -g -os -o $OUTPUT_DIR/cpptest.out \
    -DHELPER_CONSTRUCTORS \
    $OUTPUT_DIR/cpptest.cpp \
    $OUTPUT_DIR/SimpleTest.cpp \
    $OUTPUT_DIR/aligned/AutoUnionTest.cpp \
    $OUTPUT_DIR/Foo.cpp \
    $OUTPUT_DIR/Bar.cpp \
    $OUTPUT_DIR/aligned/AnkiEnum.cpp \
    $OUTPUT_DIR/ExplicitUnion.cpp \
    $OUTPUT_DIR/UnionOfUnion.cpp \
    $SUPPORTDIR/source/SafeMessageBuffer.cpp \
    $OUTPUT_DIR/DefaultValues.cpp
$OUTPUT_DIR/cpptest.out

