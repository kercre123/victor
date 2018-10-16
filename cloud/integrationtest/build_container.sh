#!/bin/bash

SCRIPT_PATH="$( cd "$(dirname "$0")" ; pwd -P )"
BUILD_PATH=${SCRIPT_PATH}/..

REGION=us-west-2
ACCOUNT_ID=649949066229
IMAGE_NAME=load_test

REPO_DNS_NAME=${ACCOUNT_ID}.dkr.ecr.${REGION}.amazonaws.com

# Delete any previous docker builds
rm -f ${SCRIPT_PATH}/robot_simulator

# Build test executable
cd ${BUILD_PATH}
./build-cloud-linux.sh -d integrationtest

# Move executable into Docker build context
cd ${SCRIPT_PATH}
cp ../../_build/cloud/integrationtest robot_simulator

docker build -t ${IMAGE_NAME} .

# Push the image to the ECR repo
$(aws-okta exec loadtest-account -- aws ecr get-login --no-include-email --region ${REGION})
docker tag ${IMAGE_NAME}:latest ${REPO_DNS_NAME}/${IMAGE_NAME}:latest
docker push ${REPO_DNS_NAME}/${IMAGE_NAME}:latest
