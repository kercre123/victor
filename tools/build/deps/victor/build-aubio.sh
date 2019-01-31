#!/usr/bin/env bash

set -e
set -u

: ${AUBIO_REVISION_TO_BUILD:="d4a1d0fb"}

echo "Building aubio ${AUBIO_REVISION_TO_BUILD} for victor ....."

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_PATH_ABSOLUTE=`pushd ${SCRIPT_PATH} >> /dev/null; pwd; popd >> /dev/null`

source ${SCRIPT_PATH_ABSOLUTE}/common-preamble.sh \
       aubio \
       https://github.com/aubio/aubio.git \
       ${AUBIO_REVISION_TO_BUILD}

cd ${BUILDDIR}/aubio

# Build for macOS
echo "Building aubio ${AUBIO_REVISION_TO_BUILD} for victor (macOS) ...."
DEST_DIR_MAC="${BUILDDIR}/mac"
./scripts/get_waf.sh
RANLIB=/usr/bin/ranlib \
AR=/usr/bin/ar \
./waf --verbose configure build install --destdir=$DEST_DIR_MAC
mv $DEST_DIR_MAC/usr/local/* $DEST_DIR_MAC
rm -r $DEST_DIR_MAC/usr

# Copy artifacts to distribution
mkdir -p ${DISTDIR}/mac
cp -av ${DEST_DIR_MAC}/include ${DISTDIR}/mac/
cp -av ${DEST_DIR_MAC}/bin ${DISTDIR}/mac/
cp -av ${DEST_DIR_MAC}/lib ${DISTDIR}/mac/

# Clean out the macOS build state before building for vicOS
echo "Cleaning out build state from macOS build"
git clean -dffx .
git submodule foreach --recursive 'git clean -dffx .'

# Build for vicOS
echo "Building aubio ${AUBIO_REVISION_TO_BUILD} for victor (vicOS) ...."
VICOS_TOOLCHAIN_ROOT=${VICOS_SDK_HOME}/prebuilt
VICOS_TOOLCHAIN_NAME=arm-oe-linux-gnueabi
VICOS_TOOLCHAIN_PREFIX=${VICOS_TOOLCHAIN_ROOT}/bin/${VICOS_TOOLCHAIN_NAME}-

VICOS_CC="${VICOS_TOOLCHAIN_PREFIX}clang"
VICOS_RANLIB="${VICOS_TOOLCHAIN_PREFIX}ranlib"
VICOS_AR="${VICOS_TOOLCHAIN_PREFIX}ar"
VICOS_NM="${VICOS_TOOLCHAIN_PREFIX}nm"

DEST_DIR_VICOS="${BUILDDIR}/vicos"
rm -rf $DEST_DIR_VICOS
./scripts/get_waf.sh

WAF_OPTS="--disable-avcodec --disable-samplerate --disable-jack --disable-sndfile --disable-accelerate"
CFLAGS="-Os" \
CC=$VICOS_CC \
RANLIB=$VICOS_RANLIB \
AR=$VICOS_AR \
NM=$VICOS_NM \
./waf --verbose configure build install \
  --notests \
  --destdir=$DEST_DIR_VICOS \
  --with-target-platform=linux \
  $WAF_OPTS

# move files to more logical locations
mv $DEST_DIR_VICOS/usr/local/* $DEST_DIR_VICOS
rm -r $DEST_DIR_VICOS/usr

# Copy artifacts to distribution directory
mkdir -p ${DISTDIR}/vicos
cp -av ${DEST_DIR_VICOS}/include ${DISTDIR}/vicos/
cp -av ${DEST_DIR_VICOS}/lib ${DISTDIR}/vicos/

# Build distribution archive
${MAKE_DEP_ARCHIVE_SH} aubio ${AUBIO_REVISION_TO_BUILD}

rm -rf ${DISTDIR}
