#!/bin/bash

set -e
set -u

CLAD="${PYTHON:=python} ${PYTHONFLAGS:=} ${EMITTER:=../../C_emitter.py}"
CLADSRC=../src
OUTPUT_DIR=${OUTPUT_DIR:-./build/simple}

for file in $CLADSRC/aligned/*.clad; do
    OUTPUT_DIR_PARAM=$(dirname $OUTPUT_DIR/${file#$CLADSRC/};)
    mkdir -p ${OUTPUT_DIR_PARAM}
    $CLAD -o ${OUTPUT_DIR_PARAM} -C $(dirname $file) $(basename $file);
done

cp -f ctest.c $OUTPUT_DIR

clang -Wall -Wextra -g -Os -o $OUTPUT_DIR/ctest.out $OUTPUT_DIR/ctest.c
$OUTPUT_DIR/ctest.out
