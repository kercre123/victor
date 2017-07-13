#!/usr/bin/env bash

set -e
set -u

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
TOPLEVEL=`pushd ${SCRIPT_PATH}/.. >> /dev/null; pwd; popd >> /dev/null`

for f in ${TOPLEVEL}/build-android-sdk/dist/*.zip;
do
    ${SCRIPT_PATH}/upload-to-s3-build-assets.sh $f
done
