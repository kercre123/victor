#!/bin/bash

#  UnitTestBuild.sh
#  BaseStation
#
#  Created by damjan stulic on 4/3/13.
#  Copyright (c) 2013 Anki. All rights reserved.

SRCDIR=$1
WORKDIR=$2

GIT=`which git`
if [ -z $GIT ]
then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`


#echo "src  dir =" $1
#echo "work dir =" $2
if [ -z $SRCDIR ] ; then
  echo "[$SRCDIR] \$1 is empty"
  exit 1;
fi

if [  ! -d $SRCDIR ] ; then
  echo "source dir [$SRCDIR] is not provided"
  exit 1;
fi

if [ -z $WORKDIR ] ; then
  echo "[$WORKDIR] \$2 is empty"
  exit 1;
fi

if [  ! -d $WORKDIR ] ; then
  echo "work dir [$WORKDIR] is not provided"
  /bin/mkdir $WORKDIR
fi

if [ -z $ASSETDIR ] ; then
  ASSETDIR="$TOPLEVEL/../drive-assets"
fi

if [  ! -d $ASSETDIR ] ; then
  echo "asset dir [$ASSETDIR] does not exist"
  exit 1;
fi

#echo "pwd = " $PWD
#echo ls SRCDIR
#/bin/ls -lap $SRCDIR
#echo ls WORKDIR
#/bin/ls -lap $WORKDIR
#echo ls WORKDIR/BaseStation
#/bin/ls -lap $WORKDIR/basestation

BASESTATION_DIR=${WORKDIR}/BaseStation

echo $TOPLEVEL/tools/build/copyResources.sh -t android -d $BASESTATION_DIR -a $ASSETDIR -b $SRCDIR -u
$TOPLEVEL/tools/build/copyResources.sh -t android -d $BASESTATION_DIR -a $ASSETDIR -b $SRCDIR -u

#echo rm -rf ${BASESTATION_DIR}
#/bin/rm -rf ${BASESTATION_DIR}
#echo mkdir ${BASESTATION_DIR}
#/bin/mkdir ${BASESTATION_DIR}
#echo cp -r $SRCDIR/resources/config ${BASESTATION_DIR}/config
#/bin/cp -r $SRCDIR/resources/config ${BASESTATION_DIR}/config
#EXITSTATUS=$?
#echo rm -rf $WORKDIR/testdata
#/bin/rm -rf $WORKDIR/testdata
#echo mkdir $WORKDIR/testdata
#/bin/mkdir $WORKDIR/testdata


exit $EXITSTATUS