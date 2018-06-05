#!/bin/bash

set -u

# Go to directory of this script
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=$(basename ${0})
GIT=`which git`
if [ -z $GIT ]
then
    echo git not found
    exit 1
fi
: ${TOPLEVEL:=`$GIT rev-parse --show-toplevel`}

# Settings can be overridden through environment
: ${ANKI_BUILD_TYPE:="Release"}
: ${BACKTRACE_UPLOAD_TOKEN:="a9730523c7a1fd68644231b0ba20012b8da9f2c7a7b12565b111933fad3f8c0a"}

function usage() {
  echo "$SCRIPT_NAME [OPTIONS]"
  echo "options:"
  echo "  -h                  print this message"
  echo "  -c ANKI_BUILD_TYPE  build configuration {Debug,Release}"
  echo ""
  echo "environment variables:"
  echo '  $ANKI_BUILD_TYPE    build configuration {Debug,Release}'
}

while getopts "hc:" opt; do
  case $opt in
    h)
      usage && exit 0
      ;;
    c)
      ANKI_BUILD_TYPE="${OPTARG}"
      ;;
    *)
      usage && exit 1
      ;;
  esac
done

echo "ANKI_BUILD_TYPE: ${ANKI_BUILD_TYPE}"

BUILD_FOLDER=${TOPLEVEL}/_build/vicos/${ANKI_BUILD_TYPE}
if [ ! -d "${BUILD_FOLDER}" ]; then
  echo "Build folder not found: "${BUILD_FOLDER}
  exit 1
fi
pushd ${BUILD_FOLDER}

# Create the symbols package (must run on Linux, not osx)
echo "Renaming *.full build artifacts..."

# Note that build server puts these all within one flat folder
rename 's/\.full$//' *.full

rm -rf symbols

echo "Generating symbols..."
DUMP_SYMS_TOOL=${TOPLEVEL}/tools/crash-tools/linux/dump_syms
for i in `ls *`; do
  echo Processing ${i}...
  $DUMP_SYMS_TOOL $i . > $i.sym || $DUMP_SYMS_TOOL -v $i > $i.sym
  VER=`head -1 $i.sym | awk '{print $4;}'`
  mkdir -p symbols/$i/$VER
  mv $i.sym symbols/$i/$VER/
done

echo "Creating compressed symbols package..."
SYMBOLS_FILE='symbols.tar.gz'
rm -f ${SYMBOLS_FILE}
tar -czf ${SYMBOLS_FILE} symbols/

echo 'Attempting to upload '${SYMBOLS_FILE}'...'

curl -v --data-binary @$SYMBOLS_FILE 'https://anki.sp.backtrace.io:6098/post?format=symbols&token='${BACKTRACE_UPLOAD_TOKEN}

popd # ${BUILD_FOLDER}

echo "Done"
