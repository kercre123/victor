#!/bin/bash

# this should be invoked by build scripts with the usage:
# aarUnityRemoval.sh IN-AARFILE OUT-AARFILE

INFILE=$1
OUTFILE=$2

TEMPDIR=$(basename $INFILE)_temp

rm -rf $TEMPDIR
mkdir $TEMPDIR

# unzip classes.jar from source aar
unzip -q $INFILE classes.jar -d $TEMPDIR

# delete all unity class files from classes.jar
zip -d $TEMPDIR/classes.jar com/unity3d/\*

# unity also adds these when it builds
zip -d $TEMPDIR/classes.jar bitter/jnibridge/\*
zip -d $TEMPDIR/classes.jar org/fmod/\*

# copy infile to outfile
cp $INFILE $OUTFILE

# update outfile with new classes.jar
zip -u -j $OUTFILE $TEMPDIR/classes.jar

# we're done
