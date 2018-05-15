#!/bin/bash

set -e
set -u

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
TOPLEVEL=`pushd ${SCRIPT_PATH}/.. >> /dev/null; pwd; popd >> /dev/null`

HOST_PLATFORM=`uname -a | awk '{print tolower($1);}' | sed -e 's/darwin/mac/'`
BREAKPAD_ANDROID_NDK_VERSION="r15b"
BREAKPAD_VICOS_VERSION="0.9-r03"

: ${BREAKPAD_REVISION_TO_BUILD:="master"}
: ${BREAKPAD_BUILD_PLATFORM:=$HOST_PLATFORM}
: ${BREAKPAD_BUILD_RUN_UNIT_TESTS:=1}

function usage() {
    echo "$0 [-h] [-p platform] [-r revision]"
    echo "-h              : help. Print out this info"
    echo "-p platform     : vicos, linux, mac, android. default is $HOST_PLATFORM or value"
    echo "                  of BREAKPAD_BUILD_PLATFORM environment variable"
    echo "-r revision     : branch name or git commit sha (like 26ed338)."
    echo "                  default is master or value of"
    echo "                  BREAKPAD_REVISION_TO_BUILD environment variable"
    echo "-s              : skip unit tests"
}

while getopts ":hp:r:s" opt; do
    case $opt in
    h)
        usage
        exit 0
        ;;
    p)
        BREAKPAD_BUILD_PLATFORM=$OPTARG
        ;;
    r)
        BREAKPAD_REVISION_TO_BUILD=$OPTARG
        ;;
    s)
        BREAKPAD_BUILD_RUN_UNIT_TESTS=0
        ;;
    :)
        echo "Option -$OPTARG requires an argument." >&2
        usage
        exit 1
        ;;
    esac
done

rm -rf build-breakpad/${BREAKPAD_BUILD_PLATFORM}/dist

mkdir -p build-breakpad/${BREAKPAD_BUILD_PLATFORM}/dist/bin

pushd build-breakpad
if [ ! -d depot_tools ]; then
    git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
fi

pushd depot_tools
./update_depot_tools
popd # depot_tools

if [ "${BREAKPAD_BUILD_PLATFORM}" = "android" ]; then
    # Set up Android NDK if necessary
    ANDROID_NDK_ROOT=`${TOPLEVEL}/tools/ankibuild/android.py --install-ndk ${BREAKPAD_ANDROID_NDK_VERSION} | tail -1`
    echo ANDROID_NDK_ROOT = $ANDROID_NDK_ROOT
    NDK_STANDALONE_DIR="${TOPLEVEL}/build-breakpad/${BREAKPAD_BUILD_PLATFORM}/ndk-standalone"
    rm -rf $NDK_STANDALONE_DIR
    $ANDROID_NDK_ROOT/build/tools/make-standalone-toolchain.sh \
        --arch=arm \
        --platform=android-19 \
        --install-dir="${NDK_STANDALONE_DIR}"

    export PATH="${NDK_STANDALONE_DIR}/bin":"$PATH"

elif [ "${BREAKPAD_BUILD_PLATFORM}" = "vicos" ]; then
    # Set up VICOS toolchain if necessary
    VICOS_SDK_ROOT=`${TOPLEVEL}/tools/ankibuild/vicos.py --install ${BREAKPAD_VICOS_VERSION} | tail -1`
    echo VICOS_SDK_ROOT = $VICOS_SDK_ROOT

    export PATH="${VICOS_SDK_ROOT}/prebuilt/bin":"$PATH"
fi

export PATH=`pwd`/depot_tools:"$PATH"

pushd ${BREAKPAD_BUILD_PLATFORM}

if [ -d src ]; then
    pushd src
    [ -e Makefile ] && make distclean
    git clean -dffx android/sample_app
    popd # src
else
    fetch breakpad
fi

gclient sync --reset -v --revision src@$BREAKPAD_REVISION_TO_BUILD

DISTDIR=`pwd`/dist
pushd src
if [ $BREAKPAD_BUILD_PLATFORM = "android" ]; then
    ./configure --host=arm-linux-androideabi \
                --disable-processor \
                --disable-tools \
                --prefix=$DISTDIR
    make install-data-am

    # Build lib using ndk-build to get a version compatible with clang and the c++_shared stl
    # Unfortunately, trying to do this with the standalone toolchain doesn't work
    pushd android/sample_app
    APP_PLATFORM=android-19 \
    APP_STL=c++_shared \
    NDK_TOOLCHAIN_VERSION=clang \
    APP_ABI=armeabi-v7a \
    $ANDROID_NDK_ROOT/ndk-build -e

    mkdir -p $DISTDIR/libs/armeabi-v7a
    cp -pv ./obj/local/armeabi-v7a/libbreakpad_client.a $DISTDIR/libs/armeabi-v7a/
    popd # android/sample_app

    # patch linux_syscall_support.h to avoid an implicit cast
    pushd $DISTDIR/include/breakpad/third_party/lss
    patch \
      linux_syscall_support.h \
      -i $DISTDIR/../../../build-scripts/patch-linux_syscall_support.h.txt
    rm linux_syscall_support.h.orig
    popd # $DISTDIR/include/breakpad/third_party/lss

    # Remove un-needed directories from the distribution
    rm -rf $DISTDIR/share $DISTDIR/lib $DISTDIR/bin
elif [ $BREAKPAD_BUILD_PLATFORM = "vicos" ]; then
    VICOS_TOOLCHAIN_ROOT=${VICOS_SDK_ROOT}/prebuilt
    VICOS_TOOLCHAIN_NAME=arm-oe-linux-gnueabi
    VICOS_TOOLCHAIN_PREFIX=${VICOS_TOOLCHAIN_ROOT}/bin/${VICOS_TOOLCHAIN_NAME}-

    VICOS_CC="CC=${VICOS_TOOLCHAIN_PREFIX}clang"
    VICOS_CXX="CXX=${VICOS_TOOLCHAIN_PREFIX}clang++"
    VICOS_RANLIB="RANLIB=${VICOS_TOOLCHAIN_PREFIX}ranlib"
    VICOS_AR="AR=${VICOS_TOOLCHAIN_PREFIX}ar"
    VICOS_NM="NM=${VICOS_TOOLCHAIN_PREFIX}nm"
    VICOS_CPPFLAGS="CPPFLAGS=-I${VICOS_TOOLCHAIN_ROOT}/include/c++/v1"
    VICOS_HOST="--host=${VICOS_TOOLCHAIN_NAME}"
    VICOS_FLAGS="$VICOS_CC $VICOS_CXX $VICOS_RANLIB $VICOS_AR $VICOS_NM $VICOS_CPPFLAGS $VICOS_HOST"
    echo VICOS_FLAGS = $VICOS_FLAGS

    BREAKPAD_FLAGS="--disable-processor --disable-tools"
    PREFIX_VICOS="--prefix=$DISTDIR"
    echo "*** Configuring Breakpad for Vicos..."
    ./configure $VICOS_FLAGS $BREAKPAD_FLAGS $PREFIX_VICOS
    echo "*** Building Breakpad for Vicos..."
    make

    make install
 
    echo "*** Cleanup..."
    mkdir -p $DISTDIR/libs/armeabi-v7a
    cp -pv $DISTDIR/lib/libbreakpad_client.a $DISTDIR/libs/armeabi-v7a/

    # patch linux_syscall_support.h to avoid an implicit cast
    pushd $DISTDIR/include/breakpad/third_party/lss
    patch \
      linux_syscall_support.h \
      -i $DISTDIR/../../../../build-scripts/patch-linux_syscall_support.h.txt
    rm linux_syscall_support.h.orig
    popd # $DISTDIR/include/breakpad/third_party/lss

    # Remove un-needed directories from the distribution
    rm -rf $DISTDIR/share $DISTDIR/lib $DISTDIR/bin
else
    ./configure --prefix=$DISTDIR
    make

    if [ $BREAKPAD_BUILD_PLATFORM = "mac" ]; then
        pushd src/tools/mac/dump_syms
        xcodebuild -target dump_syms
        cp -pv build/Release/dump_syms $DISTDIR/bin/
        popd # src/tools/mac/dump_syms
    fi

    if [ $BREAKPAD_BUILD_RUN_UNIT_TESTS -eq 1 ]; then
        make check
    fi

    make install
fi

popd # src

popd # ${BREAKPAD_BUILD_PLATFORM}

popd # build-breakpad
