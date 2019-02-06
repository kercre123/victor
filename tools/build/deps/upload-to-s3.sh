#!/usr/bin/env bash

# Assumes that credentials are available through the environment or global config files
#
# http://docs.aws.amazon.com/cli/latest/topic/config-vars.html#cli-aws-help-config-vars
#
# Specifically, the AWS Access Key and the AWS Secret Key are set

set -e
set -u

S3URL_BASE=$1
ARTIFACT_STEM=$2

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
TOPLEVEL=`pushd ${SCRIPT_PATH}/.. >> /dev/null; pwd; popd >> /dev/null`

PATH=/usr/local/bin:$PATH virtualenv venv
./venv/bin/pip install awscli

for FULLPATH in ${TOPLEVEL}/dist/${ARTIFACT_STEM}*;
do
    FILENAME=`basename $FULLPATH`
    S3URL="$S3URL_BASE/$FILENAME"

    printf "Uploading to S3\n  src: $FULLPATH\n  dst: $S3URL\n\n"
    ./venv/bin/aws s3 cp $FULLPATH $S3URL --grants read=uri=http://acs.amazonaws.com/groups/global/AllUsers
done


