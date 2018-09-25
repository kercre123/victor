#!/bin/bash

SCRIPT_PATH="$( cd "$(dirname "$0")" ; pwd -P )"
BUILD_PATH=${SCRIPT_PATH}/..

IMAGE_NAME=load_test
REPO_DNS_NAME=649949066229.dkr.ecr.us-west-2.amazonaws.com

# Delete any previous docker builds
rm -f ${SCRIPT_PATH}/integrationtest.test

# Build test executable
cd ${BUILD_PATH}
./build-cloud-linux.sh -c test -d integrationtest

# Move executable into Docker build context
cd ${SCRIPT_PATH}
cp ../../_build/cloud/integrationtest.test .

docker build -t ${IMAGE_NAME} .

# Push the image to the ECR repo
$(aws-okta exec loadtest-account -- aws ecr get-login --no-include-email --region us-west-2)
docker tag ${IMAGE_NAME}:latest ${REPO_DNS_NAME}/${IMAGE_NAME}:latest
docker push ${REPO_DNS_NAME}/${IMAGE_NAME}:latest
