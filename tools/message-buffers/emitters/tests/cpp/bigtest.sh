#!/bin/bash

set -e
set -u

CLAD="${PYTHON:=python} ${PYTHONFLAGS:=} ${EMITTER:=../../CPP_emitter.py}"
CLADSRC=../src
OUTPUT_DIR=${OUTPUT_DIR:-./build/big}
SUPPORTDIR=../../../support/cpp

echo "Generating output files"
for file in $CLADSRC/*.clad $CLADSRC/aligned/*.clad; do
    OUTPUT_DIR_PARAM=$(dirname $OUTPUT_DIR/${file#$CLADSRC/};)
    mkdir -p ${OUTPUT_DIR_PARAM}
    COMMAND="$CLAD -o ${OUTPUT_DIR_PARAM} -C $(dirname $file) $(basename $file)"
    echo $COMMAND
    time $COMMAND
done
echo "*********"
echo ""

COMMON_CFLAGS="-Wall -Wextra -Os -g -I$SUPPORTDIR/include"

cp -f cpptest.cpp $OUTPUT_DIR

echo "Compiling"
echo clang++ -std=c++11 -stdlib=libc++ $COMMON_CFLAGS -c -o $OUTPUT_DIR/SimpleTest.o $OUTPUT_DIR/SimpleTest.cpp
time clang++ -std=c++11 -stdlib=libc++ $COMMON_CFLAGS -c -o $OUTPUT_DIR/SimpleTest.o $OUTPUT_DIR/SimpleTest.cpp

SRCFILES="$OUTPUT_DIR/cpptest.cpp \
    $OUTPUT_DIR/SimpleTest.cpp \
    $OUTPUT_DIR/aligned/AnkiEnum.cpp \
    $OUTPUT_DIR/aligned/AutoUnionTest.cpp \
    $OUTPUT_DIR/Foo.cpp \
    $OUTPUT_DIR/Bar.cpp \
    $SUPPORTDIR/source/SafeMessageBuffer.cpp"

echo "clang++ -std=gnu++11 $COMMON_CFLAGS -o $OUTPUT_DIR/cpptest_gnu.out $SRCFILES"
time clang++ -std=gnu++11 $COMMON_CFLAGS -o $OUTPUT_DIR/cpptest_gnu.out $SRCFILES
echo "clang++ -stdlib=libc++ $COMMON_CFLAGS -o $OUTPUT_DIR/cpptest_clang.out $SRCFILES"
time clang++ -std=c++11 -stdlib=libc++ $COMMON_CFLAGS -o $OUTPUT_DIR/cpptest_clang.out $SRCFILES
echo "*********"
echo ""

echo "Testing"
echo $OUTPUT_DIR/cpptest_gnu.out
time $OUTPUT_DIR/cpptest_gnu.out
echo $OUTPUT_DIR/cpptest_clang.out
time $OUTPUT_DIR/cpptest_clang.out
echo "*********"
echo ""

if [ -n `which valgrind` ]; then
   echo "Memcheck"
   valgrind $OUTPUT_DIR/cpptest_clang.out
   valgrind $OUTPUT_DIR/cpptest_gnu.out
fi

