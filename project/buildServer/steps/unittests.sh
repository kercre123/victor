set -e

# change dir to the project dir, no matter where script is executed from
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "Entering directory \`${DIR}'"
cd $DIR

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL="${DIR}/../../.."

# prepare
PROJECT=$TOPLEVEL/project/gyp-mac
BUILD_TYPE="Debug"
DERIVED_DATA=$PROJECT/DerivedData

echo "Entering directory \`${PROJECT}'"
cd $PROJECT
mkdir -p $DERIVED_DATA/$BUILD_TYPE/testdata

# build
xcodebuild \
-project $PROJECT/util.xcodeproj \
-target UtilUnitTest \
-sdk macosx \
-configuration $BUILD_TYPE  \
SYMROOT="$DERIVED_DATA" \
OBJROOT="$DERIVED_DATA" \
build 

rm -f $DERIVED_DATA/$BUILD_TYPE/*.txt
rm -f $DERIVED_DATA/$BUILD_TYPE/*.xml

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
set -o pipefail
ANKIWORKROOT="$DERIVED_DATA/$BUILD_TYPE/testdata" \
ANKICONFIGROOT="$DERIVED_DATA/$BUILD_TYPE/" \
DYLD_FRAMEWORK_PATH="$DERIVED_DATA/$BUILD_TYPE/" \
DYLD_LIBRARY_PATH="$DERIVED_DATA/$BUILD_TYPE/" \
GTEST_OUTPUT=xml:$DERIVED_DATA/$BUILD_TYPE/utilGoogleTest.xml \
$TOPLEVEL/tools/build/multiTest/MultiTest.py \
--stdout_fail \
--path $DERIVED_DATA/$BUILD_TYPE \
--executable UtilUnitTest \
--stdout_file \
--xml_dir "$DERIVED_DATA/$BUILD_TYPE" \
--xml_basename "utilGoogleTest_" \
$ARGS

EXIT_STATUS=$?
set -e

if (( $DUMP_OUTPUT )); then
    cat $DERIVED_DATA/$BUILD_TYPE/*.txt
fi

#tarball files together
echo "Entering directory \`${DERIVED_DATA}/${BUILD_TYPE}}'"
cd $DERIVED_DATA/$BUILD_TYPE
tar czf utilUnitGoogleTest.tar.gz utilGoogleTest*
mv utilUnitGoogleTest.tar.gz $PROJECT/utilUnitGoogleTest.tar.gz
# exit
exit $EXIT_STATUS
