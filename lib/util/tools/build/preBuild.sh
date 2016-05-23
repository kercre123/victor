#!/bin/bash

BUILDDIR=$1
UNITTEST=$2

GIT=`which git`
if [ -z $GIT ]
then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`
echo "TOPLEVEL = $TOPLEVEL"


# build string maps
$TOPLEVEL/tools/build/stringMapBuilder/stringMapBuilder.sh $UNITTEST
if [ $? -ne 0 ]; then
  echo "string map error"
  exit 1
fi

# run json lint
pushd $TOPLEVEL
./tools/build/jsonLint/jsonLint.py
if [ $? -ne 0 ]; then
  echo "json lint error"
  exit 1
fi
popd

# run temp check
pushd $TOPLEVEL
GIT_BRANCH=`git rev-parse --abbrev-ref HEAD`
if [ $GIT_BRANCH = "master" ]; then
    FAIL_ON_TEMP="-fail"
else
    FAIL_ON_TEMP=""
fi
./tools/build/TempCheck.sh $FAIL_ON_TEMP
if [ $? -ne 0 ]; then
  echo "no TEMPs allowed"
  exit 1
fi
popd


