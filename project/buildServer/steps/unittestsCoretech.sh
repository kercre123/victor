#!/usr/bin/env bash
#
set -eu

# change dir to the script dir, no matter where we are executed from
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPTNAME=="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi

# configure

function usage() {
    echo "$SCRIPTNAME [OPTIONS]"
    echo "  -h                      print this message"
    echo "  -v                      verbose output"
    echo "  -c [CONFIGURATION]      build configuration {Debug,Release}"
    echo "  -p [PLATFORM]           build target platform {android,mac}"
}

: ${PLATFORM:=mac}
: ${CONFIGURATION:=Debug}
: ${VERBOSE:=0}

while getopts "hvc:p" opt; do
  case $opt in
    h)
      usage
      exit 1
      ;;
    v)
      VERBOSE=1
      ;;
    c)
      CONFIGURATION="${OPTARG}"
      ;;
    p)
      PLATFORM="${OPTARG}"
      ;;
    *)
      usage
      exit 1
      ;;
  esac
done

echo "Entering directory \`${SCRIPTDIR}'"
cd $SCRIPTDIR

: ${TOPLEVEL:=`$GIT rev-parse --show-toplevel`}
BUILDPATH=${TOPLEVEL}/_build/${PLATFORM}/${CONFIGURATION}/coretech

XML="*GoogleTest.xml"
LOG=ctiGoogleTest.log
LOGZIP=ctiGoogleTest.tar.gz

CTEST="ctest --output-on-failure -O ${LOG}"

if (( ${VERBOSE} )); then
  CTEST="${CTEST} -V"
fi

echo "Entering directory \`${BUILDPATH}'"
cd ${BUILDPATH}

# clean
rm -rf ${LOGZIP} ${LOG} `find * -name ${XML}`

# prepare
mkdir -p testdata

# execute
set +e
set -o pipefail

${CTEST}

EXIT_STATUS=$?

set -e

#  publish results
tar czf ${LOGZIP} ${LOG} `find * -name ${XML}`

# exit
exit $EXIT_STATUS
