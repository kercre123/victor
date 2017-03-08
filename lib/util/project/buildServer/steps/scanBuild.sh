#!/bin/bash

# change dir to the project dir, no matter where script is executed from
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "Entering directory \`${DIR}'"
cd $DIR

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi

BREW=`which brew`
if [ -z $BREW ];then
  echo brew not found
  exit 1
fi

TOPLEVEL=`$GIT rev-parse --show-toplevel`

# prepare
PROJECT=$TOPLEVEL/project/gyp-mac
BUILD_TYPE="Debug"
DERIVED_DATA=$PROJECT/DerivedData
SCAN_BUILD_OUTPUT=$DERIVED_DATA/ScanBuildOutput
mkdir -p $SCAN_BUILD_OUTPUT

SCAN_BUILD=`which scan-build`
if [ -z $SCAN_BUILD ];then
  $BREW install llvm
  export PATH=/usr/local/opt/llvm/bin:$PATH
  SCAN_BUILD=`which scan-build`
  if [ -z $SCAN_BUILD ];then
    echo no scan-build found.
    exit 1
  fi
fi
# static analyze build
$SCAN_BUILD -o $SCAN_BUILD_OUTPUT --use-analyzer Xcode \
xcodebuild \
-project $PROJECT/util.xcodeproj \
-target All \
-sdk macosx \
-configuration $BUILD_TYPE  \
SYMROOT="$DERIVED_DATA" \
OBJROOT="$DERIVED_DATA" \
build

tar -czf $DERIVED_DATA/scan_build_output.tar.gz $SCAN_BUILD_OUTPUT
