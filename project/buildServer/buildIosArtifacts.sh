#!/bin/bash

BUILDDIR=$1
UNITTEST=$2

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
"iosLibArtifact.sh" \
"resourceArtifact.sh" \
)

names=( \
"clean build folders" \
"configure project" \
"build ios lib artifact" \
"build resource artifact" \
)

for ((i=0;i<${#files[@]};++i)); do
    $TOPLEVEL/project/buildServer/steps/${files[i]}
    if [ $? -ne 0 ]; then
      echo ${names[i]} error
      exit 1
    fi
done

