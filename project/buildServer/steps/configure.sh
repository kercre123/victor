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
UTIL_TOPLEVEL="${DIR}/../../.."
REPO_TOPLEVEL=`$GIT rev-parse --show-toplevel`

cd $UTIL_TOPLEVEL/project/gyp
./configure.py --buildTools $REPO_TOPLEVEL/tools/build --with-clad $REPO_TOPLEVEL/tools/message-buffers --with-gyp $REPO_TOPLEVEL/tools/gyp --projectRoot  $UTIL_TOPLEVEL/
