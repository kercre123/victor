#!/bin/bash

SCRIPT_PATH="$( cd "$(dirname "$0")" ; pwd -P )"
BUILD_PATH=${SCRIPT_PATH}/..

echo ${BUILD_PATH}

# Delete any previous docker builds
rm -f ${SCRIPT_PATH}/integrationtest.test

# Build test executable
cd ${BUILD_PATH}
./build-cloud-linux.sh -c test -d integrationtest

# Move executable into Docker build context
cd ${SCRIPT_PATH}
cp ../../_build/cloud/integrationtest.test .

docker build -t vic-cloud-test .
