#!/bin/bash
# Copyright (c) 2014 Anki, Inc.
# All rights reserved.
set -e

function usage() {
  echo "$0 [-a]"
  echo "-a              : clean agresively"
  echo ""
  echo "If no options are given, all ignored git files are removed, exclusing xcode user project settings"
}

agressive=0
while getopts "ha" opt; do
  case $opt in
    a)
      agressive=1
      ;;
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
if [ $agressive -eq 1 ]; then
  git clean -dffx
  git submodule foreach --recursive 'git clean -dffx'
else
  for f in `git clean -ndX | egrep -v "idea|xcuserdata|vagrant" | cut -d " " -f 3`;do echo $f;rm -rf $f; done;
  git submodule foreach --recursive 'for f in `git clean -ndX | egrep -v "idea|xcuserdata" | cut -d " " -f 3`;do echo $f;rm -rf $f; done;'
fi

