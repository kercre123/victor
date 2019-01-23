#!/usr/bin/env bash

set -e
set -u

: ${LIBSODIUM_REVISION_TO_BUILD:="1.0.16"}

echo "Building libsodium ${LIBSODIUM_REVISION_TO_BUILD} for victor ....."

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_PATH_ABSOLUTE=`pushd ${SCRIPT_PATH} >> /dev/null; pwd; popd >> /dev/null`

source ${SCRIPT_PATH_ABSOLUTE}/common-preamble.sh \
       libsodium \
       git@github.com:jedisct1/libsodium.git \
       ${LIBSODIUM_REVISION_TO_BUILD}

cd ${BUILDDIR}/libsodium

# Build for macOS
echo "Building libsodium ${LIBSODIUM_REVISION_TO_BUILD} for macOS"
PREFIX=${DISTDIR}/mac
./autogen.sh
mkdir -p ${PREFIX}
RANLIB=/usr/bin/ranlib \
AR=/usr/bin/ar \
./configure \
    --disable-soname-versions \
    --enable-minimal \
    --prefix="${PREFIX}"

make clean && \
make -j3 install && \
echo "Successfully built libsodium ${LIBSODIUM_REVISION_TO_BUILD} for macOS"

# Clean and then build for vicOS
git clean -dffx .
git submodule foreach --recursive 'git clean -dffx .'

echo "Building libsodium ${LIBSODIUM_REVISION_TO_BUILD} for vicOS"
export TARGET_ARCH=armv7-a
export CFLAGS="-Os -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb -marm -march=${TARGET_ARCH}"
export ARCH=arm
export HOST_COMPILER=arm-oe-linux-gnueabi
export CC="${HOST_COMPILER}-clang"
export PREFIX="${DISTDIR}/vicos"
export PATH="${PATH}:${VICOS_SDK_HOME}/prebuilt/bin"

./autogen.sh
./configure \
    --disable-soname-versions \
    --enable-minimal \
    --host="${HOST_COMPILER}" \
    --prefix="${PREFIX}" \
    --with-sysroot="${VICOS_SDK_HOME}/sysroot" || exit 1

make clean && \
make -j3 install && \
echo "Successfully built libsodium ${LIBSODIUM_REVISION_TO_BUILD} for vicOS"


${MAKE_DEP_ARCHIVE_SH} libsodium ${LIBSODIUM_REVISION_TO_BUILD}

rm -rf ${DISTDIR}
