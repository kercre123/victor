#!/bin/bash

function branch_time_stamp {
   # Provides raw time of last commit to branch
   echo `git for-each-ref --sort='-committerdate' --format='%(committerdate:raw) %(refname)%09' refs/heads | sed -e 's-refs/heads/--' | grep "$1" | awk '{print $1}'`
}

# $1 will be %teamcity.build.branch%
BRANCH_NAME=$1

if [ "$BRANCH_NAME" != "refs/heads/master" ]; then
  set -e
  git fetch --progress origin +refs/pull/$BRANCH_NAME:$BRANCH_NAME
  PR_TIME=`branch_time_stamp $BRANCH_NAME`
  git branch -D $BRANCH_NAME

  MERGE_BRANCH=`echo $BRANCH_NAME | sed 's/\([0-9]*\)\/\(head\)/\1\/merge/'`
  git fetch --progress origin +refs/pull/$MERGE_BRANCH:$MERGE_BRANCH
  git checkout -f $MERGE_BRANCH
  git submodule update --init --recursive
  git submodule update --recursive

  MR_BRANCH_TIME=`branch_time_stamp $MERGE_BRANCH`

  if [ "$MR_BRANCH_TIME" -lt "$PR_TIME" ]; then
    echo "Github merge branch is stale. Assume MERGE CONFLICTS" 1>&2
    git checkout HEAD^
    git branch -D $MERGE_BRANCH
    exit 1
  fi
fi
