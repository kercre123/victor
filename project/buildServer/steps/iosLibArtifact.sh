set -e

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

cd $TOPLEVEL/project/iOS
#rake build:realclean
export XCODEBUILD_SCHEME="BaseStationDist"
rake build[Release,iphoneos]
rake build[Release,iphonesimulator]
rake build[Debug,iphoneos]
rake build[Debug,iphonesimulator]
unset XCODEBUILD_SCHEME

# find the build location
OBJROOT=`rake build:args | perl -pe 's/.*OBJROOT=\"(.*)\"/$1/'  | tr -d ' '`
#echo "[$OBJROOT]"

# create new folders
if [ -d $OBJROOT/artifact ]; then
  rm -rf $OBJROOT/artifact
fi
mkdir -p $OBJROOT/artifact/include
mkdir -p $OBJROOT/artifact/lib


# create fat libraries
LIPO=`which lipo`
if [ -z $LIPO ];then
  echo lipo not found
  exit 1
fi
BUILD_PRODUCT_NAME='libBaseStationDist.a'
$LIPO -create $OBJROOT/Release-iphoneos/${BUILD_PRODUCT_NAME} $OBJROOT/Release-iphonesimulator/${BUILD_PRODUCT_NAME} -output $OBJROOT/artifact/lib/libBaseStation.a
$LIPO -create $OBJROOT/Debug-iphoneos/${BUILD_PRODUCT_NAME} $OBJROOT/Debug-iphonesimulator/${BUILD_PRODUCT_NAME} -output $OBJROOT/artifact/lib/libBaseStationDebug.a

# copy include files
ditto $OBJROOT/Release-iphoneos/include $OBJROOT/artifact/include

# create tarball
cd $OBJROOT/artifact
tar czf basestationLib.tar.gz lib include
#tar xzf basestationLib.tar.gz lib include

# prepare version text file
grep "BASESTATION_VERSION " $TOPLEVEL/source/anki/basestation/version.h | cut -d \" -f 2 > $OBJROOT/artifact/version.txt
