set -e

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

# prepare
PROJECT=$TOPLEVEL/project/gyp-ios
BUILD_TYPE="Release"
DERIVED_DATA=$PROJECT/DerivedData

cd $PROJECT

# build
xcodebuild \
-project $PROJECT/util.xcodeproj \
-target All \
-sdk iphoneos \
-arch armv7 \
-configuration $BUILD_TYPE \
SYMROOT="$DERIVED_DATA" \
OBJROOT="$DERIVED_DATA" \
build
