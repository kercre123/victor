#!/usr/bin/env bash

set -e
set -u

: ${PROTOBUF_VERSION:="v3.5.1"}
: ${OPENCV_REVISION_TO_BUILD:="3.4.0"}

echo "Building opencv ${OPENCV_REVISION_TO_BUILD} for victor ....."

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
PATCHDIR=`pushd ${SCRIPT_PATH}/opencv-${OPENCV_REVISION_TO_BUILD}-patches >> /dev/null; pwd; popd >> /dev/null`
SCRIPT_PATH_ABSOLUTE=`pushd ${SCRIPT_PATH} >> /dev/null; pwd; popd >> /dev/null`

source ${SCRIPT_PATH_ABSOLUTE}/common-preamble.sh \
       opencv \
       git@github.com:opencv/opencv.git \
       ${OPENCV_REVISION_TO_BUILD}

echo "Finding/Installing protobuf ${PROTOBUF_VERSION}"
DEPTOOLPY=${TOPLEVEL}/tools/ankibuild/deptool.py
PROTOBUF_DIR=$(${DEPTOOLPY} --project victor --name protobuf --url-prefix ${S3_ASSETS_URL_PREFIX} --install ${PROTOBUF_VERSION} | tail -1)

cd ${BUILDDIR}/opencv

# Apply our Anki local patches
for f in ${PATCHDIR}/*.patch; do
    git apply $f
done

# Build opencv for mac
echo "Building opencv ${OPENCV_REVISION_TO_BUILD} for macOS ...."

mkdir build_mac
pushd build_mac

${CMAKE_EXE} \
-DBUILD_PROTOBUF=OFF \
-DPROTOBUF_UPDATE_FILES=ON \
-DProtobuf_LIBRARY=${PROTOBUF_DIR}/mac/lib/libprotobuf.a \
-DProtobuf_INCLUDE_DIR=${PROTOBUF_DIR}/mac/include \
-DProtobuf_PROTOC_EXECUTABLE=${PROTOBUF_DIR}/mac/bin/protoc \
-DCMAKE_BUILD_TYPE=Release \
-DBUILD_opencv_java=OFF \
-DBUILD_SHARED_LIBS=OFF \
-DWITH_WEBP=OFF \
-DENABLE_CXX11=ON \
-DINSTALL_C_EXAMPLES=OFF ..

make opencv_dnn -j8
make -j8

echo "Moving mac files for opencv ${OPENCV_REVISION_TO_BUILD}"

mkdir -p ${DISTDIR}/mac/lib/Release
ls
cp -vr opencv2 ${DISTDIR}/mac/
cp -vr lib/* ${DISTDIR}/mac/lib/Release/

mkdir -vp ${DISTDIR}/mac/3rdparty/lib/Release
cp -vr 3rdparty/lib/* ${DISTDIR}/mac/3rdparty/lib/Release
cp -v 3rdparty/ippicv/ippicv_mac/lib/intel64/libippicv.a ${DISTDIR}/mac/3rdparty/lib/Release
popd

# Build for vicOS
echo "Building opencv ${OPENCV_REVISION_TO_BUILD} for victor (vicOS) ...."

rm -rf build_vicos
mkdir build_vicos
pushd build_vicos

${CMAKE_EXE} \
  -DCMAKE_TOOLCHAIN_FILE=${VICOS_SDK_HOME}/cmake/vicos.oelinux.toolchain.cmake \
  -DVICOS_SDK="${VICOS_SDK_HOME}" \
  -DENABLE_NEON=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=ON \
  -DBUILD_DOCS=OFF \
  -DBUILD_FAT_JAVA_LIB=OFF \
  -DBUILD_JASPER=OFF \
  -DBUILD_OPENEXR=OFF \
  -DBUILD_PACKAGE=OFF \
  -DBUILD_PERF_TESTS=OFF \
  -DBUILD_opencv_java=OFF \
  -DBUILD_TESTS=OFF \
  -DBUILD_TIFF=OFF \
  -DBUILD_WITH_DEBUG_INFO=OFF \
  -DBUILD_opencv_apps=OFF \
  -DBUILD_opencv_java=OFF \
  -DBUILD_opencv_python2=OFF \
  -DBUILD_opencv_world=OFF \
  -DCMAKE_C_FLAGS_RELEASE="-O3 -DNDEBUG -fvisibility=hidden -ffunction-sections -fstack-protector-all" \
  -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG -fvisibility=hidden  -ffunction-sections -fstack-protector-all -fvisibility-inlines-hidden" \
  -DENABLE_PRECOMPILED_HEADERS=OFF \
  -DWITH_EIGEN=OFF \
  -DWITH_JASPER=OFF \
  -DWITH_OPENEXR=OFF \
  -DWITH_TIFF=ON \
  -DWITH_TBB=ON \
  -DWITH_CUDA=OFF \
  -DWITH_CUFFT=OFF \
  -DWITH_GTK=OFF \
  -DWITH_WEBP=OFF \
  -DWITH_CAROTENE=OFF \
  -DVICOS_CPP_FEATURES='rtti exceptions' \
  -DBUILD_PROTOBUF=OFF \
  -DPROTOBUF_UPDATE_FILES=ON \
  -DProtobuf_LIBRARY=${PROTOBUF_DIR}/vicos/lib/libprotobuf.a \
  -DProtobuf_INCLUDE_DIR=${PROTOBUF_DIR}/vicos/include \
  -DProtobuf_PROTOC_EXECUTABLE=${PROTOBUF_DIR}/mac/bin/protoc \
  -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON ..

make opencv_dnn -j8
make -j8 install

mkdir -vp ${DISTDIR}/vicos/3rdparty/lib

cp -vrf install/* ${DISTDIR}/vicos/
cp -vrf 3rdparty/lib ${DISTDIR}/vicos/3rdparty

mkdir -p ${DISTDIR}/modules

cp -vr ../modules/* ${DISTDIR}/modules
popd

${MAKE_DEP_ARCHIVE_SH} opencv ${OPENCV_REVISION_TO_BUILD}
rm -rf ${DISTDIR}
