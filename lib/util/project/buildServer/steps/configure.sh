#!/bin/bash
# Copyright (c) 2014 Anki, Inc.
# All rights reserved.
set -e

# change dir to the project dir, no matter where script is executed from
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

cd $TOPLEVEL/project/gyp
./configure.py
