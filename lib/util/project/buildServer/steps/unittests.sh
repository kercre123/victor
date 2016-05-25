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
TOPLEVEL=`$GIT rev-parse --show-toplevel`

# prepare
PROJECT=$TOPLEVEL/lib/util/project/gyp-mac
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
$TOPLEVEL/tools/build/tools/ankibuild/multiTest.py \
--stdout_fail \
--path $DERIVED_DATA/$BUILD_TYPE \
--gtest_path "$DERIVED_DATA/$BUILD_TYPE/" \
--work_path "$DERIVED_DATA/$BUILD_TYPE/testdata" \
--config_path "$DERIVED_DATA/$BUILD_TYPE/" \
--gtest_output "xml:$DERIVED_DATA/$BUILD_TYPE/basestationGoogleTest.xml" \
--executable UtilUnitTest \
--stdout_file \
--xml_dir "$DERIVED_DATA/$BUILD_TYPE" \
$ARGS

EXIT_STATUS=$?
set -e

if (( $DUMP_OUTPUT )); then
    cat $DERIVED_DATA/$BUILD_TYPE/*.txt
fi

#tarball files together
echo "Entering directory \`${DERIVED_DATA}/${BUILD_TYPE}}'"
cd $DERIVED_DATA/$BUILD_TYPE
tar czf basestationGoogleTest.tar.gz basestationGoogleTest_*

# exit
exit $EXIT_STATUS
