#!/usr/bin/env bash

# Assumes that credentials are available through the environment or global config files
#
# http://docs.aws.amazon.com/cli/latest/topic/config-vars.html#cli-aws-help-config-vars
#
# Specifically, the AWS Access Key and the AWS Secret Key are set

set -e
set -u

PATH=/usr/local/bin:$PATH virtualenv venv
./venv/bin/pip install awscli

FULLPATH=$1
FILENAME=`basename $FULLPATH`
S3URL="s3://sai-general/build-assets/$FILENAME"

printf "Uploading to S3\n  src: $FULLPATH\n  dst: $S3URL\n\n"
./venv/bin/aws s3 cp $FULLPATH $S3URL --grants read=uri=http://acs.amazonaws.com/groups/global/AllUsers

