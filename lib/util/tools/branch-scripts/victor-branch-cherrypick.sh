#!/usr/bin/env bash
#
# lib/util/tools/branch-scripts/victor-branch-cherrypick.sh
#
# Helper script to perform cherrypick from master to release branch
#
# Exit for non-zero error
set -e
#
# Exit if a needed environment variable is not set
set -u

GIT=`which git`
if [ -z $GIT ]; then
  echo git not found
  exit 1
fi

TOPLEVEL=`$GIT rev-parse --show-toplevel`
SCRIPTDIR=$TOPLEVEL/lib/util/tools/branch-scripts

# Where is picklist?
picklist=$SCRIPTDIR/picklist

# Which branch are we picking TO?
dstbranch=release/candidate

cd $TOPLEVEL
$GIT checkout $dstbranch

$SCRIPTDIR/anki-branch-cherrypick.sh $picklist
