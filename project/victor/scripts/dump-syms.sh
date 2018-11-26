#!/bin/bash

set -e
set -u

# Check that noah is installed
if [ ! -x "$(command -v noah)" ]; then
  echo "ERROR: lnav not found. Do \"brew install linux-noah/noah/noah\""
  exit 1
fi

# Go to directory of this script
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=$(basename ${0})
GIT=`which git`
if [ -z $GIT ]
then
    echo git not found
    exit 1
fi
: ${TOPLEVEL:=`$GIT rev-parse --show-toplevel`}

echo "Making symbols for the bin folder"
pushd ${TOPLEVEL}/_build/vicos/$1/bin > /dev/null
for i in `ls *.full`; do
    i=${i%.*}
    echo Processing ${i}...
    noah ${TOPLEVEL}/tools/crash-tools/linux/dump_syms $i.full . > $i.sym || noah ${TOPLEVEL}/tools/crash-tools/linux/dump_syms -v $i > $i.sym
    VER=`head -1 $i.sym | awk '{print $4;}'`
    mkdir -p ${TOPLEVEL}/_build/vicos/$1/symbols/$i/$VER
    mv $i.sym ${TOPLEVEL}/_build/vicos/$1/symbols/$i/$VER/
done
popd > /dev/null # bin

echo "Making symbols for the lib folder"
pushd ${TOPLEVEL}/_build/vicos/$1/lib > /dev/null
for i in `ls *.full`; do
    i=${i%.*}
    echo Processing ${i}...
    noah ${TOPLEVEL}/tools/crash-tools/linux/dump_syms $i.full . > $i.sym || noah ${TOPLEVEL}/tools/crash-tools/linux/dump_syms -v $i > $i.sym
    VER=`head -1 $i.sym | awk '{print $4;}'`
    mkdir -p ${TOPLEVEL}/_build/vicos/$1/symbols/$i/$VER
    mv $i.sym ${TOPLEVEL}/_build/vicos/$1/symbols/$i/$VER/
done
popd > /dev/null # bin
