#!/bin/bash

ROOT_DIR='.'

if [ ! -d $ROOT_DIR/'.git' ]; then
    echo "wrong directiory. must run from cozmo-game"
    exit -1
fi

UTIL_DIR="${ROOT_DIR}/lib/anki/cozmo-engine/tools/anki-util/"

if [ ! -d $UTIL_DIR ]; then
    echo "could not find util dir: $UTIL_DIR"
    exit -1
fi

CHECK_DIR='resources/assets/'

echo "Running json check (this should take a few seconds)..."

./$UTIL_DIR/tools/build/jsonLint/jsonLint.py $ROOT_DIR/$CHECK_DIR

if [ $? -eq 0 ]; then
    echo "OK"
else
    echo "fix your json"
fi

