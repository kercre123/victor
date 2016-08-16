#!/usr/bin/env bash
set -e

# change dir to the project dir, no matter where script is executed from
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR
CUT=`which cut`
GREP=`which grep`

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

# prepare
TESTNAME=Util
PROJECT=$TOPLEVEL/generated/mac
BUILD_TYPE="Debug"
GTEST=$TOPLEVEL/lib/util/libs/framework/
BUILDTOOLS=$TOPLEVEL/tools/build

# build
xcodebuild \
-workspace $PROJECT/CozmoWorkspace_MAC.xcworkspace \
-scheme BUILD_WORKSPACE \
-sdk macosx \
-configuration $BUILD_TYPE \
build

BUILD_DIR=`xcodebuild \
-workspace $PROJECT/CozmoWorkspace_MAC.xcworkspace \
-scheme BUILD_WORKSPACE \
-sdk macosx \
-configuration $BUILD_TYPE \
-showBuildSettings | $GREP TARGET_BUILD_DIR | $CUT -d " " -f 7`

set -o pipefail
set +e

# clean output
rm -rf $BUILD_DIR/${TESTNAME}GoogleTest*
rm -rf $BUILD_DIR/case*
rm -f $BUILD_DIR/*.txt

DUMP_OUTPUT=0
ARGS=""
while (( "$#" )); do

    if [[ "$1" == "-x" ]]; then
        DUMP_OUTPUT=1
    else
        if [[ "$ARGS" == "" ]]; then
            ARGS="--gtest_filter=$1"
        else
            ARGS="$ARGS $1"
        fi
    fi
    shift
done

if (( \! $DUMP_OUTPUT )); then
    ARGS="$ARGS --silent"
fi

# execute
$BUILDTOOLS/tools/ankibuild/multiTest.py \
--path $BUILD_DIR \
--gtest_path "$GTEST" \
--work_path "$BUILD_DIR/" \
--config_path "$BUILD_DIR/" \
--gtest_output "xml:$BUILD_DIR/${TESTNAME}GoogleTest_.xml" \
--executable ${TESTNAME}UnitTest \
--stdout_file \
--xml_dir "$BUILD_DIR" \
--xml_basename ${TESTNAME}GoogleTest_ \
$ARGS

EXIT_STATUS=$?
set -e

if (( $DUMP_OUTPUT )); then
    cat $BUILD_DIR/*.txt
fi

#tarball files together
cd $BUILD_DIR
tar czf ${TESTNAME}GoogleTest.tar.gz ${TESTNAME}GoogleTest_*
mv ${TESTNAME}GoogleTest.tar.gz $TOPLEVEL/build/${TESTNAME}GoogleTest.tar.gz
# exit
exit $EXIT_STATUS
