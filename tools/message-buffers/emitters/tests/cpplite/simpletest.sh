#!/bin/bash

set -eu

CLAD="${PYTHON:=python} ${PYTHONFLAGS:=} ${EMITTER:=../../CPPLite_emitter.py}"
CLADSRC=../src
OUTPUT_DIR=${OUTPUT_DIR:-./build/simple}

for file in $CLADSRC/aligned-lite/*.clad; do
    OUTPUT_DIR_PARAM=$(dirname $OUTPUT_DIR/${file#$CLADSRC/};)
    mkdir -p ${OUTPUT_DIR_PARAM}
    $CLAD --max-message-size 1420 -o ${OUTPUT_DIR_PARAM} -C $(dirname $file) $(basename $file);
done

cp -f cpplitetest.cpp $OUTPUT_DIR

clang++ -DCLAD_DEBUG -Wall -Wextra -g -Os -o $OUTPUT_DIR/cpplitetest.out $OUTPUT_DIR/cpplitetest.cpp $OUTPUT_DIR/aligned-lite/CTest.cpp
$OUTPUT_DIR/cpplitetest.out
