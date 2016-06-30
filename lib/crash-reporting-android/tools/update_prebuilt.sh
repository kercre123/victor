#!/bin/bash
set -e
set -u

TOPLEVEL=`git rev-parse --show-toplevel`
pushd "${TOPLEVEL}/SharedLib" > /dev/null 2>&1
ndk-build -s breakpadclient
cp -p obj/local/armeabi-v7a/libbreakpadclient.a "${TOPLEVEL}/prebuilt/armeabi-v7a/"
popd > /dev/null 2>&1

