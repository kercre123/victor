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
  echo "PR branch created at $HUMAN_PR_BRANCH"
  git branch -D $BRANCH_NAME


  MERGE_BRANCH=`echo $BRANCH_NAME | sed 's/\([0-9]*\)\/\(head\)/\1\/merge/'`
  git_checkout $MERGE_BRANCH

  HUMAN_MERGE_TIME=`human_print_time_stamp $MERGE_BRANCH`
  echo "Merge branch created at $HUMAN_MERGE_TIME"
  MR_BRANCH_TIME=`branch_time_stamp $MERGE_BRANCH`

  # github appears to call the create(asynchronous) for */head & */merge at the same time.
  # So a margin of error needs to be added.
  # Typically there is a 200~300 milisecond delta.
  let TIME_AND_MARGIN_OF_ERROR=$PR_TIME-1500
  echo "$BRANCH_NAME was last touched at $HUMAN_PR_BRANCH"
  echo "$MERGE_BRANCH was last updated at $HUMAN_MERGE_TIME"
  HUMAN_ERROR=`human_print_time_stamp $TIME_AND_MARGIN_OF_ERROR`
  echo "$MR_BRANCH_TIME must be greater than $TIME_AND_MARGIN_OF_ERROR for merge branch to be assumed good." 

  while [ "$MR_BRANCH_TIME" -lt "$TIME_AND_MARGIN_OF_ERROR" ]; do
    echo "Github merge branch is stale. Assume MERGE CONFLICTS" 1>&2

    sleep 30
    let COUNTER=COUNTER+1

    git checkout HEAD^
    git branch -D $MERGE_BRANCH
    git_checkout $MERGE_BRANCH
    MR_BRANCH_TIME=`branch_time_stamp $MERGE_BRANCH`

    if [ $COUNTER -gt $RETRIES ]; then
      echo "Retried $RETRIES times. Exiting now."
      exit 1
    fi
  done
fi
