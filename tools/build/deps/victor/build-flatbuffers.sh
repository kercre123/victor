#!/usr/bin/env bash

set -e
set -u

: ${FLATBUFFERS_REVISION_TO_BUILD="v1.5.0"}

echo "Building flatbuffers ${FLATBUFFERS_REVISION_TO_BUILD} for victor ....."

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_PATH_ABSOLUTE=`pushd ${SCRIPT_PATH} >> /dev/null; pwd; popd >> /dev/null`

source ${SCRIPT_PATH_ABSOLUTE}/common-preamble.sh \
       flatbuffers \
       https://github.com/google/flatbuffers \
       ${FLATBUFFERS_REVISION_TO_BUILD}

# Build host prebuilts
HOST_PREBUILTS_DIR=${DISTDIR}/host-prebuilts
mkdir -p ${HOST_PREBUILTS_DIR}
pushd ${HOST_PREBUILTS_DIR}
mkdir ${FLATBUFFERS_REVISION_TO_BUILD}
ln -s ${FLATBUFFERS_REVISION_TO_BUILD} current
HOST_PREBUILTS_CURRENT_DIR=${HOST_PREBUILTS_DIR}/current
popd

# Do our macOS builds
mkdir -p ${HOST_PREBUILTS_CURRENT_DIR}/x86_64-apple-darwin/bin

# Build the host tools for macos
echo "Building flatc and flathash standalone tools for macOS"
pushd ${BUILDDIR}
${SCRIPT_PATH_ABSOLUTE}/build-flatc-host-tools.sh \
                       ${FLATBUFFERS_REVISION_TO_BUILD} \
                       ${CMAKE_EXE} \
                       ${HOST_PREBUILTS_CURRENT_DIR}/x86_64-apple-darwin/bin/

# Cleanup the build of the host tools
# Build for macOS
echo "Build flatbuffers ${FLATBUFFERS_REVISION_TO_BUILD} for victor (macOS)"
pushd flatbuffers
mkdir build
pushd build
CC=/usr/bin/clang CXX=/usr/bin/clang++ ${CMAKE_EXE} -G Xcode ..
xcodebuild -project FlatBuffers.xcodeproj -target ALL_BUILD build -configuration Release
xcodebuild -project FlatBuffers.xcodeproj -target ALL_BUILD build -configuration Debug
mkdir ${DISTDIR}/mac
mv Release ${DISTDIR}/mac/
mv Debug ${DISTDIR}/mac/
popd

# Build for vicOS
echo "Build flatbuffers ${FLATBUFFERS_REVISION_TO_BUILD} for victor (vicOS)"
mkdir vicos
pushd vicos
${CMAKE_EXE} \
    -G "Unix Makefiles" \
    -DCMAKE_TOOLCHAIN_FILE=${VICOS_SDK_HOME}/cmake/vicos.oelinux.toolchain.cmake \
    -DVICOS_SDK="${VICOS_SDK_HOME}" \
    -DFLATBUFFERS_BUILD_TESTS=OFF \
    -j8 \
    ..
make
mkdir -p ${DISTDIR}/vicos
cp libflatbuffers.a ${DISTDIR}/vicos/
popd

# Copy the includes
echo "Copy the flatbuffers include directory to the distribution directory"
cp -pvR ./include ${DISTDIR}/

# Build the host-tools for Linux using Docker
echo "Build the flatc and flathash standalone tools for Linux using Docker"
mkdir -p ${HOST_PREBUILTS_CURRENT_DIR}/x86_64-linux-gnu/bin

docker \
    build \
    -t build-flatc-host-tools \
    -f ${SCRIPT_PATH_ABSOLUTE}/build-flatc-host-tools.dockerfile \
    ${SCRIPT_PATH_ABSOLUTE}

docker \
    run \
    --rm -v ${HOST_PREBUILTS_CURRENT_DIR}/x86_64-linux-gnu/bin:/build/out \
    build-flatc-host-tools \
    /build/build-flatc-host-tools.sh v1.5.0 cmake /build/out/


${MAKE_DEP_ARCHIVE_SH} flatbuffers ${FLATBUFFERS_REVISION_TO_BUILD}

rm -rf ${DISTDIR}
