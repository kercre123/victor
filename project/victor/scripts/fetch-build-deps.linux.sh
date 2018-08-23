#!/bin/bash
set -e
set -u

GIT=`which git`
if [ -z $GIT ]; then
  echo git not found
  exit 1
fi
: ${TOPLEVEL:=`$GIT rev-parse --show-toplevel`}

function vlog()
{
    echo "[fetch-build-deps] $*"
}

function check_dep()
{
    CHECK_CMD="$*"
    eval $CHECK_CMD && return 0

    echo "Depdendency check failed: $CHECK_CMD"
    echo "If you have apt-get, you should make sure you have the following deps"
    echo ""
    echo "sudo apt install git-core gnupg flex bison gperf build-essential zip curl zlib1g-dev \\"
    echo "                 gcc-multilib g++-multilib libc6-dev-i386 lib32ncurses5-dev x11proto-core-dev \\"
    echo "		   libx11-dev lib32z-dev libxml-simple-perl libc6-dev libgl1-mesa-dev tofrodos \\"
    echo "                 python-markdown libxml2-utils xsltproc genisoimage"
    echo ""
    echo "sudo apt install gawk chrpath texinfo p7zip-full android-tools-fsutils"
    echo ""
    echo "sudo apt install ruby ninja-build subversion libssl-dev nodejs"
    echo ""
    exit 1
}

pushd "${TOPLEVEL}" > /dev/null 2>&1

# Check for required programs
check_dep which python2
check_dep which python3
check_dep which ninja

echo `pwd`

vlog "vicos-sdk"
./tools/build/tools/ankibuild/vicos.py --install 0.9-r03

vlog "CMake"
./tools/build/tools/ankibuild/cmake.py

vlog "Go"
./tools/build/tools/ankibuild/go.py

vlog "protobuf"                                                                                     
./tools/build/tools/ankibuild/protobuf.py --install

vlog "Build output dirs"
mkdir -p generated
mkdir -p _build

vlog "Fetch & extract external dependencies. This may take 1-5 min."
./project/buildScripts/dependencies.py -v --deps-file DEPS --externals-dir EXTERNALS

vlog "Configure audio library"
./lib/audio/configure.py

popd > /dev/null 2>&1
