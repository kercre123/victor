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
TOPLEVEL=`$GIT rev-parse --show-toplevel`

# prepare
PROJECT=$TOPLEVEL/project/gyp-mac
BUILD_TYPE="Debug"
DERIVED_DATA=$PROJECT/DerivedData
SONAR_OUTPUT=$DERIVED_DATA/SonarOutput
mkdir -p $SONAR_OUTPUT

# build
SONAR_WRAPPER="/opt/sonar-build-wrapper/macosx-x86/build-wrapper-macosx-x86"
$SONAR_WRAPPER --out-dir $SONAR_OUTPUT \
xcodebuild \
-project $PROJECT/util.xcodeproj \
-target All \
-sdk macosx \
-configuration $BUILD_TYPE  \
SYMROOT="$DERIVED_DATA" \
OBJROOT="$DERIVED_DATA" \
build

cd $TOPLEVEL
SONAR_RUNNER="/opt/sonar-runner/bin/sonar-runner"
$SONAR_RUNNER -e -X

