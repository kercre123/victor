#!/bin/bash

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi

PLIST_BUDDY="/usr/libexec/PlistBuddy"

TOPLEVEL=`$GIT rev-parse --show-toplevel`

version=`cat $TOPLEVEL/VERSION`
major=`echo $version | cut -d. -f1`
minor=`echo $version | cut -d. -f2`
revision=`echo $version | cut -d. -f3`
printf -v  LONG_VERSION "%05d.%05d.%05d" $major $minor $revision

ENGINE_VERSION_FILE=$TOPLEVEL/engine/buildVersion.h
SDK_VERSION_FILE=$TOPLEVEL/tools/sdk/cozmoclad/src/cozmoclad/__init__.py
PLIST_FILE=$TOPLEVEL/unity/ios/Info.plist
UNITY_VERSION_FILE=$TOPLEVEL/unity/Cozmo/ProjectSettings/ProjectSettings.asset
# All build related changes to the following version files must be ignored.

sed -i '' -e "s/\(.*kBuildVersion = \"\)\(.*\)\(\";.*\)/\1$LONG_VERSION\3/g" $ENGINE_VERSION_FILE
echo Set engine version to $LONG_VERSION

sed -i '' -e "s/\(__build_version__ = \"\)\(.*\)\(\".*\)/\1$LONG_VERSION\3/g" $SDK_VERSION_FILE
echo Set sdk build version to $LONG_VERSION

sed -i '' -e "s/\(__version__ = \"\)\(.*\)\(\".*\)/\1$version\3/g" $SDK_VERSION_FILE
echo Set sdk version to $version

if [ -f ${PLIST_BUDDY} ];then
  ${PLIST_BUDDY} -c "Set :CFBundleShortVersionString ${version}" ${PLIST_FILE}
  sed -i '' -e "s/\(bundleVersion: \)\(.*\)\(.*\)/\1$version\3/g" $UNITY_VERSION_FILE
  echo Set ios version to $version
fi
