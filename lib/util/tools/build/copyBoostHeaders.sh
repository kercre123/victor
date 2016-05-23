#!/bin/bash
#  copyHeaders.sh
#
#  Created by damjan stulic on 4/4/14.
#   Optimized by pauley on 6/20/14
#  Copyright (c) 2014 Anki. All rights reserved.

#src dir = /Users/damjan/workspace/drive-basestation/project/iOS/../../
#dst dir = /Users/damjan/Library/Developer/Xcode/DerivedData/BaseStation-ehnageqqncydccghgjhzbecfkyok/Build/Products/Debug-iphoneos

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SRCDIR="$1"/source/anki/
VEHICLESRCDIR="$1"/resources/firmware/releases/
BOOSTDIR="$1"/libs/packaged/include
DSTDIR="$2"/include

if [ ! -d "${DSTDIR}" ]; then
  if [ -a "${DSTDIR}" ]; then
    echo "Removing corrupted include dir!"
    rm -f "${DSTDIR}"
  fi
  mkdir -p "${DSTDIR}"
fi

# copy boost headers
echo "[Copy Boost Headers]"
rsync -r -t -m -p --chmod="Du=rwx,go=rx,Fuog=r" --delete --delete-excluded --include="*.h" --include="*.hpp" "${BOOSTDIR}"/boost "${DSTDIR}"
if [ $? -ne 0 ]; then
  echo "Error: copying boost headers"
  exit 1;
fi


