#!/bin/bash

set -e
set -u

CLAD="${PYTHON:=python} ${PYTHONFLAGS:=} ${EMITTER:=../../C_emitter.py}"
CLADSRC=../src
OUTPUT_DIR=${OUTPUT_DIR:-./build/big}

echo "Generating output files"
for file in $CLADSRC/aligned/*.clad; do
    OUTPUT_DIR_PARAM=$(dirname $OUTPUT_DIR/${file#$CLADSRC/};)
    mkdir -p ${OUTPUT_DIR_PARAM}
    COMMAND="$CLAD -o ${OUTPUT_DIR_PARAM} -C $(dirname $file) $(basename $file)"
    echo $COMMAND
    time $COMMAND
done
echo "*********"
echo ""

COMMON_CFLAGS="-Wall -Wextra -Os -g"

cp -f ctest.c $OUTPUT_DIR

SRCFILES="$OUTPUT_DIR/ctest.c"
STDS="gnu89 c99 gnu99 c11 gnu11"
#c89 is really rough and c94 doesn't seem to be valid
#STDS="c89 gnu89 c94 c99 gnu99 c11 gnu11"

echo "Compiling under many standards to ensure compatibility"
for std in $STDS; do
    COMMAND="clang -std=$std $COMMON_CFLAGS -o $OUTPUT_DIR/ctest_$std.out $SRCFILES"
    echo $COMMAND
    time $COMMAND
done
echo "*********"
echo ""

echo "Testing"
for std in $STDS; do
    COMMAND="$OUTPUT_DIR/ctest_$std.out"
    echo $COMMAND
    time $COMMAND
done
echo "*********"
echo ""

if [ -n `which valgrind` ]; then
    echo "Memcheck"
    for std in $STDS; do
        COMMAND="valgrind $OUTPUT_DIR/ctest_$std.out"
        echo $COMMAND
        $(COMMAND)
    done
    echo "*********"
    echo ""
fi

