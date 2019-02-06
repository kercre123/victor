#!/usr/bin/env bash

set -e
set -u

REVISION_TO_BUILD=$1
CMAKE_EXE=$2
OUTPUT_PATH=$3

if [ ! -d flatbuffers ]; then
    git clone https://github.com/google/flatbuffers
fi

pushd flatbuffers
git checkout $REVISION_TO_BUILD
git submodule update --init --recursive


mkdir -p build
pushd build
# build flatc and flathash
${CMAKE_EXE} -G"Unix Makefiles" \
    -DFLATBUFFERS_BUILD_TESTS=OFF \
    -DFLATBUFFERS_INSTALL=OFF \
    -DFLATBUFFERS_BUILD_FLATLIB=OFF \
    ..

make

cp -pv flatc flathash ${OUTPUT_PATH}
popd
rm -rf build
popd

