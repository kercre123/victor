#!/bin/bash

BUILDDIR=$1
UNITTEST=$2

GIT=`which git`
if [ -z $GIT ]
then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`
echo "TOPLEVEL = $TOPLEVEL"


# # build character emotion map from fmod project
# # needs to run BEFORE string map builder
# pushd $TOPLEVEL
# ./tools/build/characters/parse_character_vo_from_fmod.py
# if [ $? -ne 0 ]; then
# echo "character vo error"
# exit 1
# fi
# popd

# # build string maps
# $TOPLEVEL/tools/build/stringMapBuilder/stringMapBuilder.sh $UNITTEST
# if [ $? -ne 0 ]; then
#   echo "string map error"
#   exit 1
# fi

# # run json lint
# pushd $TOPLEVEL
# ./tools/build/jsonLint/jsonLint.py
# if [ $? -ne 0 ]; then
#   echo "json lint error"
#   exit 1
# fi
# popd

# # run temp check
# pushd $TOPLEVEL
# GIT_BRANCH=`git rev-parse --abbrev-ref HEAD`
# if [ $GIT_BRANCH = "master" ]; then
#     FAIL_ON_TEMP="-fail"
# else
#     FAIL_ON_TEMP=""
# fi
# ./tools/build/TempCheck.sh $FAIL_ON_TEMP
# if [ $? -ne 0 ]; then
#   echo "no TEMPs allowed"
#   exit 1
# fi
# popd

# # unpack libs
# pushd $TOPLEVEL/libs/packaged
# ./unpackLibs.sh
# if [ $? -ne 0 ]; then
#   echo "unpack libs error"
#   exit 1
# fi
# popd

# generate version file
$TOPLEVEL/tools/build/versionGenerator/versionGenerator.sh
if [ $? -ne 0 ]; then
  echo "generate version error"
  exit 1
fi

# not sure if I need this. It seemsed to spam the wrong directory with "include"
# # copy headers
# #if [ ! $UNITTEST ]; then
#   $TOPLEVEL/tools/build/copyHeaders.sh $TOPLEVEL $BUILDDIR
#   if [ $? -ne 0 ]; then
#     echo "copy headers error"
#     exit 1
#   fi
# #fi


