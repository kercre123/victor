#!/usr/bin/env bash

set -e
set -u

: ${OPUS_REVISION_TO_BUILD:="e04e86e0"}

echo "Building opus ${OPUS_REVISION_TO_BUILD} for victor ....."

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_PATH_ABSOLUTE=`pushd ${SCRIPT_PATH} >> /dev/null; pwd; popd >> /dev/null`

source ${SCRIPT_PATH_ABSOLUTE}/common-preamble.sh \
       opus \
       git@github.com:xiph/opus.git \
       ${OPUS_REVISION_TO_BUILD}

cd ${BUILDDIR}/opus

# Build for macOS
echo "Building opus ${OPUS_REVISION_TO_BUILD} for victor (macOS) ...."
./autogen.sh
PREFIX="${BUILDDIR}/mac"
mkdir -p "${PREFIX}"
RANLIB=/usr/bin/ranlib \
AR=/usr/bin/ar \
CFLAGS=-O3 ./configure --prefix=${PREFIX} --enable-fixed-point
make install

# Copy desired artifacts to distribution
mkdir -p "${DISTDIR}/mac/lib"
cp -av "${BUILDDIR}/mac/lib/libopus.a" "${DISTDIR}/mac/lib/"
cp -av "${BUILDDIR}/mac/include" "${DISTDIR}/mac/"

# Clean out the macOS build state before building for vicOS
echo "Cleaning out build state from macOS build"
git clean -dffx .
git submodule foreach --recursive 'git clean -dffx .'

# Build for vicOS
echo "Building opus for victor (vicOS) ...."
VICOS_TOOLCHAIN_ROOT=${VICOS_SDK_HOME}/prebuilt
VICOS_TOOLCHAIN_NAME=arm-oe-linux-gnueabi
VICOS_TOOLCHAIN_PREFIX=${VICOS_TOOLCHAIN_ROOT}/bin/${VICOS_TOOLCHAIN_NAME}-

VICOS_CC="CC=${VICOS_TOOLCHAIN_PREFIX}clang"
VICOS_RANLIB="RANLIB=${VICOS_TOOLCHAIN_PREFIX}ranlib"
VICOS_AR="AR=${VICOS_TOOLCHAIN_PREFIX}ar"
VICOS_NM="NM=${VICOS_TOOLCHAIN_PREFIX}nm"
VICOS_HOST="--host=${VICOS_TOOLCHAIN_NAME}"
VICOS_FLAGS="$VICOS_CC $VICOS_RANLIB $VICOS_AR $VICOS_NM $VICOS_HOST"

./autogen.sh
PREFIX="${BUILDDIR}/vicos"
mkdir -p "${PREFIX}"
CFLAGS=-O3 ./configure --prefix=${PREFIX} $VICOS_FLAGS --enable-fixed-point
make install

# Copy artifacts to distribution directory
mkdir -p "${DISTDIR}/vicos/lib"
cp -av "${BUILDDIR}/vicos/lib/"libopus* "${DISTDIR}/vicos/lib/"
cp -av "${BUILDDIR}/vicos/include" "${DISTDIR}/vicos/"

${MAKE_DEP_ARCHIVE_SH} opus ${OPUS_REVISION_TO_BUILD}

rm -rf ${DISTDIR}
