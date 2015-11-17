#!/bin/bash
# Copyright (c) 2014 Anki, Inc.
# All rights reserved.
set -e

function usage() {
    echo "$0 [-e coretech_external_path]"
    echo "-e coretech_external_path     : location of coretech external"
    echo "                                you can allso use $CORETECH_EXTERNAL_DIR environment variable"
    echo ""
    echo "runs project's configure command"
}

CTE=
while getopts "he:" opt; do
  case $opt in
    h)
      usage
      exit 1
      ;;
    e)
      CTE=$OPTARG
      ;;
  esac
done


# change dir to the project dir, no matter where script is executed from
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

if [ -z $CTE ]; then
  if [ -z $CORETECH_EXTERNAL_DIR ]; then
    echo please provide coretech external path
    usage
    exit 1
  else
    CTE=$CORETECH_EXTERNAL_DIR
  fi
fi

cd $TOPLEVEL/project/gyp
./configure.py --platform mac --coretechExternal $CTE
