#!/usr/bin/env bash

set -e
set -u

DIST_NAME=$1
DIST_REVISION=$2

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
TOPLEVEL=`pushd ${SCRIPT_PATH}/.. >> /dev/null; pwd; popd >> /dev/null`
DISTTOP=${TOPLEVEL}/dist

echo "${DIST_REVISION}" > ${DISTTOP}/${DIST_NAME}/VERSION

cd ${DISTTOP}

DIST_STEM=${DIST_NAME}-${DIST_REVISION}
DIST_TAR_BZ2=${DIST_STEM}.tar.bz2
DIST_SHA256=${DIST_STEM}-SHA-256.txt

echo "Making ${DIST_TAR_BZ2} ....."
tar -cjvf ${DIST_TAR_BZ2} ${DIST_NAME}

echo "Making ${DIST_SHA256} ....."
shasum -a 256 ${DIST_TAR_BZ2} > ${DIST_SHA256}
SHA256SUM=`cat ${DIST_SHA256} | awk '{print $1};'`
SIZE=`stat -f %z ${DIST_TAR_BZ2}`

cat <<EOF
${DIST_NAME}_info =
{
  "${DIST_REVISION}" : {"size": $SIZE, "sha256": "$SHA256SUM"},
}
EOF

