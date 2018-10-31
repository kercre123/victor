#!/bin/bash

set -e

SCRIPT_NAME=`basename ${0}`
SCRIPT_PATH="$( cd "$(dirname "$0")" ; pwd -P )"
BUILD_PATH=${SCRIPT_PATH}/..

AWS_REGION=${AWS_REGION:-us-west-2}
ACCOUNT_ID=649949066229
IMAGE_NAME=load_test
SKIP_UPLOAD=false
LOCAL_MODE=false

function usage() {
    echo "$SCRIPT_NAME [OPTIONS]"
    echo "  -h                      print this message"
    echo "  -r [AWS_REGION]         AWS region (defaults to '${AWS_REGION}')"
    echo "  -a [ACCOUNT_ID]         AWS account ID (defaults to '${ACCOUNT_ID}')"
    echo "  -i [IMAGE_NAME]         name for the target Docker image name (defaults to '${IMAGE_NAME}')"
    echo "  -l                      local development mode (docker-compose)"
    echo "  -s                      skips Docker image upload to AWS ECR"
}

while getopts "i:a:r:hsl" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        r)
            AWS_REGION="${OPTARG}"
            ;;
        a)
            ACCOUNT_ID="${OPTARG}"
            ;;
        i)
            IMAGE_NAME="${OPTARG}"
            ;;
        s)
            SKIP_UPLOAD=true
            ;;
        l)
            LOCAL_MODE=true
            ;;
        :)
            echo "Option -${OPTARG} required an argument." >&2
            usage
            exit 1
            ;;
    esac
done

REPO_DNS_NAME=${ACCOUNT_ID}.dkr.ecr.${AWS_REGION}.amazonaws.com

# Delete any previous docker builds
rm -f ${SCRIPT_PATH}/robot_simulator

# Build test executable
cd ${BUILD_PATH}
./build-cloud-linux.sh -d integrationtest

# Move executable into Docker build context
cd ${SCRIPT_PATH}
cp ../../_build/cloud/integrationtest robot_simulator

if [ "$LOCAL_MODE" = true ] ; then
    docker-compose build
    exit
fi

docker build -t ${IMAGE_NAME} .

if [ "$SKIP_UPLOAD" = false ] ; then
    # Push the image to the ECR repo
    $(aws-okta exec loadtest-account -- aws ecr get-login --no-include-email --region ${AWS_REGION})
    docker tag ${IMAGE_NAME}:latest ${REPO_DNS_NAME}/${IMAGE_NAME}:latest
    docker push ${REPO_DNS_NAME}/${IMAGE_NAME}:latest
fi
