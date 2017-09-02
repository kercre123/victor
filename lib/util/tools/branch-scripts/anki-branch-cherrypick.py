#!/bin/bash

if [ -z "$1" ]
  then
    echo "No argument supplied"
    exit 1
fi

fileName="$1"

set -e
for sha in `cat "$fileName"`; do
   echo cherry-picking sha $sha
   git cherry-pick -x $sha
   echo
done

git status -bs

echo "git push when you are done"
