set -e

# change dir to the project dir, no matter where script is executed from
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`


cd $TOPLEVEL/project/iOS
#rake build:realclean
OBJROOT=`rake test:basestation:args | perl -pe 's/.*OBJROOT=\"(.*)\"/$1/'  | tr -d ' '`
#echo "[$OBJROOT]"

#BUILD_TYPE="Release"
BUILD_TYPE="Debug"
OUTPUT_DIR=$OBJROOT/$BUILD_TYPE
rm -f $OUTPUT_DIR/basestationGoogleTest_*
rm -f $OUTPUT_DIR/googleTest_*
set +e
rake test:basestation[$BUILD_TYPE,xml,$OUTPUT_DIR,ProdDataTest]
#rake test:run[$BUILD_TYPE,xml,$OUTPUT_DIR]
EXIT_STATUS=$?
set -e

#tarball files together
cd $OUTPUT_DIR

tar czf basestationGoogleTest.tar.gz basestationGoogleTest_*
#tar czf googleTest.tar.gz googleTest_*

exit $EXIT_STATUS
