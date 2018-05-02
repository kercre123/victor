#!/bin/bash
set -x
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

pushd "${TOPLEVEL}" > /dev/null 2>&1

$GIT config --global url."git@github.com:".insteadOf https://github.com

#vlog "Check brew installation."
is_brew=`which brew`
set -e

if [ -z "$is_brew" ]; then
   echo "Brew not found installing now.  You will be prompted."
   /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
fi

vlog "Check homebrew dependencies"
./tools/build/tools/ankibuild/installBuildDeps.py \
    -d python2 \
    ninja \
    python3 \
    graphviz \
    --pip2 graphviz \
    --pip3 graphviz

vlog "Android SDK"
./tools/build/tools/ankibuild/android.py --install-sdk r3

vlog "vicos sdk"
./tools/build/tools/ankibuild/vicos.py --install 0.9-r03

vlog "CMake"
./tools/build/tools/ankibuild/cmake.py

vlog "Go"
./tools/build/tools/ankibuild/go.py

vlog "Build output dirs"
mkdir -p generated
mkdir -p _build

vlog "Fetch & extract external dependencies. This may take 1-5 min."
./project/buildScripts/dependencies.py -v --deps-file DEPS --externals-dir EXTERNALS

vlog "Configure audio library"
./lib/audio/configure.py

popd > /dev/null 2>&1
