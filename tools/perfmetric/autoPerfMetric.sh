#!/bin/bash

set -u

# Go to directory of this script
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=$(basename ${0})
GIT=`which git`
if [ -z $GIT ]
then
    echo git not found
    exit 1
fi
: ${TOPLEVEL:=`$GIT rev-parse --show-toplevel`}
LOCAL_DUMP_FOLDER=${SCRIPT_PATH}/dumps

set -e

source ${TOPLEVEL}/project/victor/scripts/victor_env.sh

# Settings can be overridden through environment
: ${VERBOSE:="0"}
: ${MANUAL_START:="0"}
: ${PRE_WAIT_TIME_SECONDS:=0}
: ${RECORDING_TIME_SECONDS:=30}
: ${VIC_PERFMETRIC_FOLDER:="/data/data/com.anki.victor/cache/perfMetric"}

function usage() {
  echo "$SCRIPT_NAME [OPTIONS]"
  echo "options:"
  echo "  -h                  print this message"
  echo "  -m                  manually start recording (script will wait for keypress)"
  echo "  -p <seconds>        pre-wait time: number of seconds to wait before recording (cannot use with -m)"
  echo "  -s ANKI_ROBOT_HOST  hostname or ip address of robot (or localhost for webots sim)"
  echo "  -t <seconds>        time (duration, in seconds) to record"
  echo "  -v                  verbose"
  echo ""
  echo "environment variables:"
  echo '  $ANKI_ROBOT_HOST    hostname or ip address of robot'
}

while getopts "hmp:s:t:v" opt; do
  case $opt in
    h)
      usage && exit 0
      ;;
    m)
      MANUAL_START="1"
      ;;
    p)
      PRE_WAIT_TIME_SECONDS="${OPTARG}"
      ;;
    s)
      ANKI_ROBOT_HOST="${OPTARG}"
      ;;
    t)
      RECORDING_TIME_SECONDS="${OPTARG}"
      ;;
    v)
      VERBOSE="1"
      ;;
    *)
      usage && exit 1
      ;;
  esac
done

robot_set_host

echo "ANKI_ROBOT_HOST: ${ANKI_ROBOT_HOST}"

if [ $MANUAL_START -eq "1" ]; then
  if [ $PRE_WAIT_TIME_SECONDS -gt 0 ]; then
    echo "Cannot specify a manual start along with a pre-wait time"
    usage && exit 1
  else
    read -n 1 -s -r -p "Press any key to begin recording >"
    echo ""
  fi
else
  if [ $PRE_WAIT_TIME_SECONDS -gt 0 ]; then
    echo "Waiting "$PRE_WAIT_TIME_SECONDS" seconds before starting to record"
    sleep $PRE_WAIT_TIME_SECONDS
  fi
fi

echo "Starting PerfMetric recording for ${RECORDING_TIME_SECONDS} seconds"

# Note: if host is not running victor this will end immediately
cmd='perfmetric?start'
returnString=$(curl -s "${ANKI_ROBOT_HOST}:8888/$cmd")
if [[ $returnString = *"Not found"* ]]; then
  echo "Failed to send perfmetric command(s)"
  echo "The build of Victor running on ${ANKI_ROBOT_HOST} may not have PerfMetric enabled (ANKI_PERF_METRIC_ENABLED)"
  exit 1
fi
sleep ${RECORDING_TIME_SECONDS}
cmd='perfmetric?stop&dumplogall&dumpfiles&dumpresponse'
if [ ${VERBOSE} -eq "1" ]; then
  cmd=${cmd}all
fi
curl -s ${ANKI_ROBOT_HOST}:8888/$cmd

echo "Done"
echo "Results also dumped to log, and to .csv and .txt files on host (cache/perfMetricLogs)"

# Below is sample script code for pulling latest file out of a folder.
# We could do something like this to pull the latest dump file(s) off
# of the robot (after a "dumpfiles" PerfMetric command)
#echo "Finding perfmetric file on robot at "$ANKI_ROBOT_HOST
#    # List all regular (non directory) files whose size is greater than zero, and sort by time
#    cmd="cd ${VIC_CRASH_FOLDER}; find . -type f -size +0c -exec ls -t {} + -o -name . -o -prune | head -1"
#    CRASH_DUMP_FILE="$(ssh root@${ANKI_ROBOT_HOST} $cmd)"
#    if [ -z ${CRASH_DUMP_FILE} ]; then
#      echo "No valid crash dump file found on robot"
#      exit 1
#    fi
#  fi
#  scp -p root@${ANKI_ROBOT_HOST}:${VIC_CRASH_FOLDER}/${CRASH_DUMP_FILE} ${LOCAL_CRASH_FOLDER}/
