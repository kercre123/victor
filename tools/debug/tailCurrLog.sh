#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

playbackLogs="$DIR/../../_build/mac/Debug/playbackLogs"

if [ ! -d ${playbackLogs} ]; then
  echo "No playback logs in ${playbackLogs}"
  exit 1
fi

pushd $playbackLogs > /dev/null

latestlogs=""

devLoggerDirs="`find . -type d -name devLogger`"

for devLoggerDir in ${devLoggerDirs} ; do
  latestDir="`find ${devLoggerDir} -type d -maxdepth 1 -mindepth 1 | cut -c 3- | sort | tail -n 1`"
  if [ "${latestDir}" != "" ]; then
    latestLogs="${latestLogs} ${latestDir}/print/*.log"
  fi
done

if [ "${latestLogs}" != "" ]; then
  tail -f ${latestLogs}
fi

popd > /dev/null

