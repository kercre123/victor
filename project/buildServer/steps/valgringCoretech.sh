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
BUILDTOOLS=$TOPLEVEL/tools/anki-util/tools/build-tools

$BUILDTOOLS/tools/ankibuild/valgrindTest.py --projectName coretech/project/gyp/coretech-internal --targetName cti --projectRoot $TOPLEVEL
