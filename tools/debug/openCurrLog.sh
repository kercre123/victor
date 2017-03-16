#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

GAME_LOG_DIR=$DIR/../../build/mac/Debug/playbackLogs/webotsCtrlGameEngine/gameLogs/devLogger/

# If the number 2 is passed in as the first arg, the log for webotsCtrlGameEngine2 will be displayed
if [ $1 -eq 2 ]; then
  GAME_LOG_DIR=$DIR/../../build/mac/Debug/playbackLogs/webotsCtrlGameEngine2/gameLogs/devLogger/
fi

pushd $GAME_LOG_DIR

LOGDIR=`find . -type d -maxdepth 1 -mindepth 1 | cut -c 3- | sort | tail -n 1`

if [ -d $LOGDIR ];
then
    open $LOGDIR/print/*.log
else
    print "could not find any logs"
fi

popd
