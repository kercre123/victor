#!/bin/bash

UNITTEST=$1

GIT=`which git`
if [ -z $GIT ]
then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

WORKDIR=$TOPLEVEL/tools/build/stringMapBuilder/
SOURCEDIR=$TOPLEVEL/resources/
CONFIG=$WORKDIR/stringMapBuilder.json
if [ -n "$UNITTEST" ]; then
  CONFIG=$WORKDIR/stringMapBuilderTest.json
fi

cd $WORKDIR
./stringMapBuilder.exe $SOURCEDIR $WORKDIR $CONFIG
