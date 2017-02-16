#!/bin/bash

set -e
set -u

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TOPLEVEL=$DIR/..

cd $TOPLEVEL/unix
make

cd $TOPLEVEL/android
./gradlew assembleRelease

cd $TOPLEVEL/ios
rm -rf tmpbuild
mkdir tmpbuild
xcodebuild -target dasTest CONFIGURATION_BUILD_DIR=./tmpbuild build
cd tmpbuild
DYLD_FALLBACK_FRAMEWORK_PATH=. ./dasTest
cd ..
rm -rf tmpbuild
