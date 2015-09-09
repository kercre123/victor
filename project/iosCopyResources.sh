#!/bin/bash
set -e
SRCDIR="$1"
ASSETSRDIR="$2"
DSTDIR="$3"

# change dir to the project dir, no matter where script is executed from
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

cd $TOPLEVEL

SPHINXDIR="$TOPLEVEL"/generated/resources/pocketsphinx
DSTDIR_META=$DSTDIR/cozmo_resources/config/
DSTDIR_SPHINX=$DSTDIR/cozmo_resources/pocketsphinx/
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

if [ ! -d "$SPHINXDIR" ]; then
  echo sphinx assets not found "$SPHINXDIR"
  exit 1;
fi

if [ -z "DSTDIR" ] ; then
	echo "[$DSTDIR] is empty"
	exit 1;
fi

# copy resources
mkdir -p "$DSTDIR_META"
rsync -r -t --exclude=".*" --delete $SRCDIR/ $DSTDIR_META/
rsync -r -t --exclude=".*" --delete $SPHINXDIR/ $DSTDIR_SPHINX/

# copy assets
if [ -d "$ASSETSRDIR" ] ; then
  mkdir -p "$DSTDIR_ASSET"
  rsync -r -t --exclude=".*" --delete $ASSETSRDIR/ $DSTDIR_ASSET/
fi


echo asset: $DSTDIR_ASSET
echo meta: $DSTDIR_META