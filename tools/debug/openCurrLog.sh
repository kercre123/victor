#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

GAME_LOG_DIR=$DIR/../../build/mac/Debug/playbackLogs/webotsCtrlGameEngine/gameLogs/

pushd $GAME_LOG_DIR

LOGDIR=`find . -type d -maxdepth 1 -mindepth 1 | cut -c 3- | sort | tail -n 1`

if [ -d $LOGDIR ];
then
    open $LOGDIR/print/*.log
else
    print "could not find any logs"
fi

popd
