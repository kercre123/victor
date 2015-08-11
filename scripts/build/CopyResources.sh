#!/bin/bash
set -e

SRCDIR="$1"
ASSETSRDIR="$2"
DSTDIR="$3"
DSTDIR_META=$DSTDIR/cozmo_resources/config/
DSTDIR_ASSET=$DSTDIR/cozmo_resources/assets/

# verify inputs
if [ -z "$SRCDIR" ] ; then
	echo "[$SRCDIR] is empty"
	exit 1;
fi

if [ -z "$ASSETSRDIR" ] ; then
	echo "[$ASSETSRDIR] is empty"
#	exit ;
fi

if [ ! -d "$SRCDIR" ] ; then
	echo "source dir [$SRCDIR] is not provided"
	exit 1;
fi

if [ -z "DSTDIR" ] ; then
	echo "[&DSTDIR] is empty"
	exit 1;
fi

# copy resources
mkdir -p "$DSTDIR_META"
rsync -r -t --exclude=".*" --delete $SRCDIR/ $DSTDIR_META/

# copy assets
if [ -d "$ASSETSRDIR" ] ; then
  mkdir -p "$DSTDIR_ASSET"
  rsync -r -t --exclude=".*" --delete $ASSETSRDIR/ $DSTDIR_ASSET/
fi
echo asset: $DSTDIR_ASSET
echo meta: $DSTDIR_META