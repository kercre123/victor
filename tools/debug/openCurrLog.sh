#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

playbackLogs="$DIR/../../_build/mac/Debug/playbackLogs"

if [ ! -d ${playbackLogs} ]; then
  echo "No playback logs in ${playbackLogs}"
  exit 1
fi

pushd $playbackLogs > /dev/null

devLoggerDirs="`find . -type d -name devLogger`"

for devLoggerDir in ${devLoggerDirs} ; do
  pushd ${devLoggerDir} > /dev/null
  latest=`find . -type d -maxdepth 1 -mindepth 1 | cut -c 3- | sort | tail -n 1`
  open ${latest}/print/*.log
  popd > /dev/null
done

popd > /dev/null
