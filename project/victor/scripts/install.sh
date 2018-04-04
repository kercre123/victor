#!/bin/bash

set -e
set -u

# Go to directory of this script
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=$(basename ${0})


VERBOSE=""
KEEP=false
function usage() {
  echo "$SCRIPT_NAME [OPTIONS] BUILD_DIR DIST_DIR"
  echo "options:"
  echo "  -h                      print this message"
  echo "  -v                      verbose logging"
  echo "  -k                      keep destination directory, don't clear it before installing"
  echo ""
  echo "required arguments:"
  echo "  BUILD_DIR               directory containing build artifacts"
  echo "  DIST_DIR                directory that will contain distribution artifacts for robot"
  echo ""
}

while getopts "hvk" opt; do
  case $opt in
    h)
        usage && exit 0
        ;;
    v)
        VERBOSE="-v"
        ;;
    k)
        KEEP=true
        ;;
    *)
        usage && exit 1
        ;;
  esac
done

shift $((OPTIND -1))

if [ $# -lt 2 ]; then
    usage
    exit 1
fi

pushd $1 > /dev/null 2>&1
BUILDDIR=`pwd`
popd > /dev/null 2>&1

if [ $KEEP == false ]; then
  rm -rf $2
fi
mkdir -p $2/anki
pushd $2/anki > /dev/null 2>&1
DISTDIR=`pwd`
popd > /dev/null 2>&1

pushd ${BUILDDIR} > /dev/null 2>&1
# Build list of files for distribution
DIST_LIST="dist.$$.lst"
rm -f "dist.*.lst"
touch ${DIST_LIST}

find lib -type f -name '*.so' >> ${DIST_LIST}
find bin -type f -not -name '*.full' >> ${DIST_LIST}
find etc >> ${DIST_LIST}
find data >> ${DIST_LIST}

# Copy files to distribution directory
rsync \
    ${VERBOSE} \
    -r \
    -l \
    -t \
    -D \
    --files-from=${DIST_LIST} \
    ./ \
    ${DISTDIR}/


rm -f ${DIST_LIST}
popd > /dev/null 2>&1
