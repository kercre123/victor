#!/bin/bash
# Copyright (c) 2014 Anki, Inc.
# All rights reserved.
set -e

function usage() {
  echo "$0 "
  echo ""
  echo "cleans all uncommited and ignored files from the repository"
}

while getopts "h" opt; do
  case $opt in
    h)
      usage
      exit 1
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

cd $TOPLEVEL

# Clean the workspace
git clean -dffx
git submodule foreach --recursive 'git clean -dffx'

