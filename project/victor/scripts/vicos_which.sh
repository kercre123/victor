#!/usr/bin/env bash

if [ $# -eq 0 ]; then
	echo "Need an argument"
	exit 1
fi

GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
VICOS_SDK=`${GIT_PROJ_ROOT}/tools/build/tools/ankibuild/vicos.py --find 0.9-r03`

if [ ! -d "$VICOS_SDK" ]; then
	echo "Could not find vicos SDK"
	exit 1
fi

VICOS_BIN=$VICOS_SDK/prebuilt/bin
LINUX_VERSION=$VICOS_BIN/arm-oe-linux-gnueabi-$1
if [ -f "$LINUX_VERSION" ]; then
	echo "$LINUX_VERSION"
	exit 0
fi

if [ -f "$VICOS_BIN/$1" ]; then
	echo "$VICOS_BIN/$1"
	exit 0
fi

echo "Could not find executable $1 in $VICOS_BIN"
exit 1
