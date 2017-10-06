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

: ${TOPLEVEL:=`$GIT rev-parse --show-toplevel`}

# configure
PROJECT=${TOPLEVEL}/coretech
PLATFORM=mac
CONFIG=Debug

BUILDPATH=${TOPLEVEL}/_build/${PLATFORM}/${CONFIG}/coretech
LOG=ctiGoogleTest.log
LOGZIP=ctiGoogleTest.tar.gz

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
mv ${LOGZIP} ${PROJECT}/${LOGZIP}

# exit
exit $EXIT_STATUS
