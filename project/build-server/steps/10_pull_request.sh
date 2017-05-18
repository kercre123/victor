#!/bin/bash

function branch_time_stamp {
   # Provides raw time of last commit to branch
   echo `git for-each-ref --sort='-committerdate' --format='%(committerdate:raw) %(refname)%09' refs/heads | sed -e 's-refs/heads/--' | grep "$1" | awk '{print $1}'`
}

function human_print_time_stamp {
   # Provides raw time of last commit to branch
   echo `git for-each-ref --sort='-committerdate' --format='%(committerdate:iso8601) %(refname)%09' refs/heads | sed -e 's-refs/heads/--' | grep "$1" | awk '{print $1,$2}'`
}

function git_checkout {
  git fetch --depth=3 --prune --force --progress origin +refs/pull/$1:$1
  git checkout -f $1
  git submodule update --init --recursive
  git submodule update --recursive
}


# $1 will be %teamcity.build.branch%
BRANCH_NAME=$1
RETRIES=5
COUNTER=0

if [ "$BRANCH_NAME" != "refs/heads/master" ]; then
  set -e
  git fetch --depth=1 --prune --force --progress origin +refs/pull/$BRANCH_NAME:$BRANCH_NAME
  HUMAN_PR_BRANCH=`human_print_time_stamp $BRANCH_NAME`
  PR_TIME=`branch_time_stamp $BRANCH_NAME`
  git branch -D $BRANCH_NAME


  MERGE_BRANCH=`echo $BRANCH_NAME | sed 's/\([0-9]*\)\/\(head\)/\1\/merge/'`
  git_checkout $MERGE_BRANCH

  HUMAN_BRANCH_TIME=`human_print_time_stamp $MERGE_BRANCH`
  MR_BRANCH_TIME=`branch_time_stamp $MERGE_BRANCH`

  # github appears to call the create(asynchronous) for */head & */merge at the same time.
  # So a margin of error needs to be added.
  # Typically there is a 200~300 milisecond delta.
  let TIME_AND_MARGIN_OF_ERROR=$PR_TIME-1000

  while [ "$MR_BRANCH_TIME" -lt "$TIME_AND_MARGIN_OF_ERROR" ]; do
    echo "Github merge branch is stale. Assume MERGE CONFLICTS" 1>&2
    echo "$BRANCH_NAME/head was last touched at $HUMAN_BRANCH_TIME"
    echo "merge branch was last updated at $HUMAN_BRANCH_TIME"

    sleep 30
    let COUNTER=COUNTER+1

    git checkout HEAD^
    git branch -D $MERGE_BRANCH
    git_checkout $MERGE_BRANCH
    HUMAN_BRANCH_TIME=`human_print_time_stamp $MERGE_BRANCH`
    MR_BRANCH_TIME=`branch_time_stamp $MERGE_BRANCH`

    if [ $COUNTER -gt $RETRIES ]; then
      exit 1
    fi
  done
fi
