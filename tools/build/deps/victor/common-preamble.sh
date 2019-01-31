#!/usr/bin/env bash

set -e
set -u

DEPNAME=${1}
GITURL=${2}
GITREV=${3}

: ${VICOS_SDK_VERSION:="1.1.0-r04"}
: ${S3_ASSETS_URL_PREFIX:="https://sai-general.s3.amazonaws.com/build-assets/deps/victor/"}

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_PATH_ABSOLUTE=`pushd ${SCRIPT_PATH} >> /dev/null; pwd; popd >> /dev/null`

export S3_ASSETS_URL_PREFIX
export TOPLEVEL=`pushd ${SCRIPT_PATH}/../.. >> /dev/null; pwd; popd >> /dev/null`
export BUILDDIR=${TOPLEVEL}/build-victor-deps/build-${DEPNAME}
export DISTTOP=${TOPLEVEL}/dist
export DISTDIR=${DISTTOP}/${DEPNAME}
export MAKE_DEP_ARCHIVE_SH=${TOPLEVEL}/deps/make-dep-archive.sh

echo "Finding/Installing vicos-sdk ${VICOS_SDK_VERSION}"
VICOSPY=${TOPLEVEL}/tools/ankibuild/vicos.py
export VICOS_SDK_HOME=$(${VICOSPY} --install ${VICOS_SDK_VERSION} | tail -1)

echo "Finding/Installing cmake 3.9.6 ......"
CMAKEPY=${TOPLEVEL}/tools/ankibuild/cmake.py
export CMAKE_EXE=$(${CMAKEPY} --install-cmake 3.9.6 | tail -1)

# Create a fresh and clean build directory
rm -rf ${BUILDDIR}
mkdir -p ${BUILDDIR}

# Clone source code from git
cd ${BUILDDIR}
git clone ${GITURL}

# Checkout the desired revision
cd ${DEPNAME}
git checkout ${GITREV}
git submodule update --init --recursive
