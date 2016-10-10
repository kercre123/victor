#!/bin/bash

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi

TOPLEVEL=`$GIT rev-parse --show-toplevel`

version=`cat $TOPLEVEL/VERSION`
major=`echo $version | cut -d. -f1`
minor=`echo $version | cut -d. -f2`
revision=`echo $version | cut -d. -f3`
printf -v  LONG_VERSION "%05d.%05d.%05d" $major $minor $revision

ENGINE_VERSION_FILE=$TOPLEVEL/cozmoAPI/include/anki/cozmo/buildVersion.h
SDK_VERSION_FILE=$TOPLEVEL/tools/sdk/cozmoclad/src/cozmoclad/__init__.py
# All build related changes to the following version files must be ignored.

sed -i '' -e "s/\(.*kBuildVersion = \"\)\(.*\)\(\";.*\)/\1$LONG_VERSION\3/g" $ENGINE_VERSION_FILE
echo Set engine version to $LONG_VERSION

sed -i '' -e "s/\(__build_version__ = \"\)\(.*\)\(\".*\)/\1$LONG_VERSION\3/g" $SDK_VERSION_FILE
echo Set sdk build version to $LONG_VERSION

sed -i '' -e "s/\(__version__ = \"\)\(.*\)\(\".*\)/\1$version\3/g" $SDK_VERSION_FILE
echo Set sdk version to $version

#TODO: Change plist.  Change androidmanifest.xml.
