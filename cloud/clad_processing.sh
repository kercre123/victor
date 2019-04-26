#!/bin/bash

CLAD_WORKING_DIR=../clad
CLAD_INPUT_DIR=src
CLAD_OUTPUT_DIR=../generated/cladgo/src

cd ${CLAD_WORKING_DIR}

for filename in ${CLAD_INPUT_DIR}/clad/cloud/*.clad
do
    echo "Generating code for $filename"
    ../victor-clad/tools/message-buffers/emitters/Go_emitter.py -C ${CLAD_INPUT_DIR} -o ${CLAD_OUTPUT_DIR} clad/cloud/$(basename $filename)
done
