#!/bin/bash

set -e
set -u

CLAD="${PYTHON:=python} ${PYTHONFLAGS:=} ${EMITTER:=../../Python_emitter.py}"
CLADSRC=../src
OUTPUT_DIR=${OUTPUT_DIR:-./build/simple}

for file in $CLADSRC/*.clad $CLADSRC/aligned/*.clad; do
    OUTPUT_DIR_PARAM=$(dirname $OUTPUT_DIR/${file#$CLADSRC/};)
    mkdir -p ${OUTPUT_DIR_PARAM}
    $CLAD -o ${OUTPUT_DIR_PARAM} -C $(dirname $file) $(basename $file);
done

touch $OUTPUT_DIR/aligned/__init__.py

cp -f pythontest.py $OUTPUT_DIR
$PYTHON $PYTHONFLAGS $OUTPUT_DIR/pythontest.py
