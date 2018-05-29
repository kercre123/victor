#!/usr/bin/env bash

# Script to easily create Behavior Stack Paths file for Audio Designers


# Get Project root
GIT_PROJ_ROOT=`git rev-parse --show-toplevel`

echo "path $GIT_PROJ_ROOT"

# Build Mac app to run test
_status=$($GIT_PROJ_ROOT/project/victor/build-victor.sh -p mac -f)
if [ "$_status" ]; then
    echo "Success building Victor Mac project"
    echo "Run test to generate behavior path file"

    # Only run necessary test
    export GTEST_FILTER=DelegationTree.DumpBehaviorTreeBranchesToFile
    _status=$($GIT_PROJ_ROOT/project/buildServer/steps/unittestsEngine.sh)

    if [ "$_status" ]; then
        # Open file
        _behaviorFile="$GIT_PROJ_ROOT/_build/mac/Debug/test/engine/behavior_branches.txt"
        echo "Created file: '$_behaviorFile'"
        open $_behaviorFile
    else
        echo "Test Failed to run"
        exit 1
    fi

else
    echo "Error building Victor Mac project"
    exit 1
fi

exit 0
