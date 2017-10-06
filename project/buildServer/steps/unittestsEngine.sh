#!/usr/bin/env bash
#
set -e

# change dir to the script dir, no matter where we are executed from
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "Entering directory \`${SCRIPTDIR}'"
cd $SCRIPTDIR

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi

: ${TOPLEVEL:=`$GIT rev-parse --show-toplevel`}

# configure
PROJECT=${TOPLEVEL}/test
PLATFORM=mac
CONFIG=Debug
BUILDPATH=${TOPLEVEL}/_build/${PLATFORM}/${CONFIG}/test
LOG=cozmoEngineGoogleTest.log
LOGZIP=cozmoEngineGoogleTest.tar.gz

echo "Entering directory \`${BUILDPATH}'"
cd ${BUILDPATH}

# clean
rm -rf ${LOG} ${LOGZIP} ${PROJECT}/${LOGZIP}

# prepare
mkdir -p testdata

# execute
set +e
set -o pipefail
ctest -V -O ${LOG}

EXIT_STATUS=$?
set -e

#  publish results
tar czf ${LOGZIP} ${LOG}
mv ${LOGZIP} ${PROJECT}

# exit
exit $EXIT_STATUS
