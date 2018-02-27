#!/bin/bash

set -eu

GIT=`which git`
if [ -z $GIT ]
then
    echo git not found
    exit 1
fi

TOPLEVEL=`$GIT rev-parse --show-toplevel`

cd $TOPLEVEL/tools/victor-ble-cli
node index.js

