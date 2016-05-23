#!/bin/bash

BUILDDIR=$1
UNITTEST=$2

function usage() {
  echo "$0 [-n]"
  echo "-n              : do not clean project"
  echo ""
  echo "runs continuous integration scripts, like build server would"
}

# change dir to the project dir, no matter where script is executed from
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

GIT=`which git`
if [ -z $GIT ]
then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`


files=( \
"clean.sh" \
"configure.sh" \
"unittests.sh" \
"valgrindTest.sh" \
"singleIosTarget.sh" \
"androidLib.sh" \
)

names=( \
"clean build folders" \
"configure project - run gyp" \
"build and run basestion osx google unittests" \
"runs valgrind on single unittest" \
"build basestion lib ios relese" \
"build basestion lib android" \
)


while getopts "hn" opt; do
  case $opt in
    n)
      files=("${files[@]:1}")
      names=("${names[@]:1}")
      ;;
    h)
      usage
      exit 1
      ;;
  esac
done


for ((i=0;i<${#files[@]};++i)); do
  $TOPLEVEL/project/buildServer/steps/${files[i]}
  if [ $? -ne 0 ]; then
    echo ${names[i]} error
    exit 1
  fi
done
