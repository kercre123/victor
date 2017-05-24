#!/usr/bin/env bash

# Dump ANKI_BUILD variables
# ( set -o posix ; set ) | grep "ANKI_BUILD"

set -e
set -o pipefail

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))            
_SCRIPT_NAME=`basename ${0}`

VERBOSE=0
CONFIGURE=0
: ${TARGET_ARCH="arm64"}
: ${OUTPUT_DIR:="build_libunity-generated"}
: ${CMAKE_EXE:=`which cmake`}

function usage() {
    echo "$_SCRIPT_NAME [options]"
    echo "-h            print this help message"
    echo "-v            verbose output"
    echo "-a ARCH       target architecture (armv7,arm64)"
    echo "-o OUTPUT_DIR output directory (default: build_libunityplayer.\$ARCH)"
}

while getopts ":a:o:c:hv" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        v)
            VERBOSE=1
            ;;
        a)
            TARGET_ARCH="${OPTARG}"
            ;;
        c)
            CMAKE_EXE="${OPTARG}"
            ;;
        o)
            OUTPUT_DIR="${OPTARG}"
            ;;
        :)
            echo "Options -${OPTARG} requires an argument." >&2
            usage
            exit 1
            ;;
    esac
done


pushd ${ANKI_BUILD_REPO_ROOT}

pushd build/ios/unity-ios/${CONFIGURATION}-iphoneos

if [ ! -f CMakeLists.txt ]; then
  ln ${ANKI_BUILD_REPO_ROOT}/unity/ios/unity-generated/unity-generated.cmake CMakeLists.txt
fi

BUILD_DIR="${OUTPUT_DIR}"

mkdir -p ${BUILD_DIR}

pushd ${BUILD_DIR}

${CMAKE_EXE} .. \
    -GNinja \
    -DCMAKE_TOOLCHAIN_FILE=${ANKI_BUILD_REPO_ROOT}/unity/ios/unity-generated/ios-toolchain-llvm.cmake

ninja

popd

popd

popd
