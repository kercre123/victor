set -e
TESTNAME=cti
PROJECTNAME=coretech-internal
PROJECTROOT=
# change dir to the project dir, no matter where script is executed from
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "Entering directory \`${DIR}'"
cd $DIR

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`
BUILDTOOLS=$TOPLEVEL/tools/anki-util/tools/build-tools

# prepare
PROJECT=$TOPLEVEL/$PROJECTROOT/generated/mac
BUILD_TYPE="Debug"
DERIVED_DATA=$PROJECT/DerivedData

# build
xcodebuild \
-project $PROJECT/coretech/project/gyp/$PROJECTNAME.xcodeproj \
-target ${TESTNAME}UnitTest \
-sdk macosx \
-configuration $BUILD_TYPE  \
SYMROOT="$DERIVED_DATA" \
OBJROOT="$DERIVED_DATA" \
build 


set -o pipefail
set +e

# clean output
rm -rf $DERIVED_DATA/$BUILD_TYPE/${TESTNAME}GoogleTest*

# execute
#ANKIWORKROOT="$DERIVED_DATA/$BUILD_TYPE/testdata" \
#ANKICONFIGROOT="$DERIVED_DATA/$BUILD_TYPE/" \
#echo \
DYLD_FRAMEWORK_PATH="$DERIVED_DATA/$BUILD_TYPE/" \
DYLD_LIBRARY_PATH="$DERIVED_DATA/$BUILD_TYPE/" \
GTEST_OUTPUT=xml:$DERIVED_DATA/$BUILD_TYPE/${TESTNAME}GoogleTest_.xml \
$BUILDTOOLS/tools/ankibuild/multiTest.py \
--silent \
--path $DERIVED_DATA/$BUILD_TYPE \
--executable ${TESTNAME}UnitTest \
--stdout_file \
--xml_dir "$DERIVED_DATA/$BUILD_TYPE" \
--xml_basename ${TESTNAME}GoogleTest_

EXIT_STATUS=$?
set -e

#tarball files together
cd $DERIVED_DATA/$BUILD_TYPE
tar czf ${TESTNAME}GoogleTest.tar.gz ${TESTNAME}GoogleTest_*

# exit
exit $EXIT_STATUS
