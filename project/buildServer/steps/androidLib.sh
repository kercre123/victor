set -e

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

# prepare
PROJECT="${TOPLEVEL}/project/gyp-android"
BUILD_TYPE="Release"
NINJA_DIR="${PROJECT}/out/${BUILD_TYPE}"

cd "${NINJA_DIR}"
ninja #boom


