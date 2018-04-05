#!/bin/bash

if [ "$#" == "0" ]; then
  echo "Usage: $0 <name of .so> <address> [<address>]"
  exit 1
fi

GIT_PROJ_ROOT=`git rev-parse --show-toplevel`

if [ -z "${ANDROID_NDK+x}" ]; then
  ANDROID_NDK=`${GIT_PROJ_ROOT}/tools/build/tools/ankibuild/android.py`
fi

configuration=Release
shared_library=$1
shift

${ANDROID_NDK}/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin/arm-linux-androideabi-addr2line -p -e ${GIT_PROJ_ROOT}/_build/android/${configuration}/lib/${shared_library}.full -a $@
