#!/bin/bash
#
# Get latest SDK nexclad branch, try to run SDK tests against cozmo-one master code with Webots
#

set -e
set -u

PYTHON=`which python3`
if [ -z $PYTHON ];then
  echo python not found
  exit 1
fi

export SDK_TCP_PORT=5106

_SCRIPT_PATH=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)
_TOPLEVEL_COZMO=${_SCRIPT_PATH}/../../..
_SDK_WEBOTS_SCRIPT=${_TOPLEVEL_COZMO}/project/build-scripts/webots/sdkTest.py

NUM_RUNS_PARAM=""
if [[ ${NUM_RUNS:-} ]];then
  NUM_RUNS_PARAM="--numRuns $NUM_RUNS"
fi
SDK_TIMEOUT_PARAM=""
if [[ ${SDK_TIMEOUT:-} ]];then
  SDK_TIMEOUT_PARAM="--timeout $SDK_TIMEOUT"
fi
SDK_TEST_CONFIG_PARAM=""
if [[ ${SDK_TEST_CONFIG:-} ]];then
  SDK_TEST_CONFIG_PARAM="--configFile $SDK_TEST_CONFIG"
fi
$PYTHON $_SDK_WEBOTS_SCRIPT $SDK_TIMEOUT_PARAM $NUM_RUNS_PARAM $SDK_TEST_CONFIG_PARAM
exit $?
