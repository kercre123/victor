#!/bin/bash

set -e
set -u

CLAD="${PYTHON:=python} ${PYTHONFLAGS:=} ${EMITTER:=../../CSharp_emitter.py}"
CLADSRC=../src
OUTPUT_DIR=${OUTPUT_DIR:-./build/simple}

for file in $CLADSRC/*.clad $CLADSRC/aligned/*.clad; do
    OUTPUT_DIR_PARAM=$(dirname $OUTPUT_DIR/${file#$CLADSRC/};)
    mkdir -p ${OUTPUT_DIR_PARAM}
    $CLAD -o ${OUTPUT_DIR_PARAM} -C $(dirname $file) $(basename $file);
done

cp -f csharptest.cs $OUTPUT_DIR

mcs -debug+ -out:$OUTPUT_DIR/csharptest.exe \
    $OUTPUT_DIR/csharptest.cs \
    $OUTPUT_DIR/SimpleTest.cs \
    $OUTPUT_DIR/aligned/AnkiEnum.cs \
    $OUTPUT_DIR/Bar.cs \
    $OUTPUT_DIR/Foo.cs

mono --debug $OUTPUT_DIR/csharptest.exe
