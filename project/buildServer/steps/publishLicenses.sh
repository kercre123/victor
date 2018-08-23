#!/bin/bash

# Note: requires environment variables:
#  SAI_PATH referencing https://github.com/anki/sai-viclicensefile-upload-tool/blob/master/vic-file.py
#  AWS_ACCESS_KEY_ID
#  AWS_SECRET_KEY
# also requires: pip2 install boto

GIT=`which git`
if [ -z $GIT ]
then
    echo git not found
    exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

# TODO: VIC-5668 Add version numbers to licenses report and uploads

python ${SAI_PATH}/vic-file.py --up --env prod --ver 1.0.0 --type license ${TOPLEVEL}/_build/vicos/Release/licences
