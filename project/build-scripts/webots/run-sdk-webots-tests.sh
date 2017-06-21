#!/bin/bash
#
# Get latest SDK nexclad branch, try to run SDK tests against cozmo-one master code with Webots
#

set -e
set -u

PIP=`which pip3`
if [ -z $PIP ];then
  echo pip not found
  exit 1
fi

PYTHON=`which python3`
if [ -z $PYTHON ];then
  echo python not found
  exit 1
fi

export SDK_TCP_PORT=5106

_SCRIPT_PATH=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)
_TOPLEVEL_COZMO=${_SCRIPT_PATH}/../../..
_COZMO_REPO_DIR=${_TOPLEVEL_COZMO}/tools/sdk/cozmo-python-sdk
_SDK_WEBOTS_SCRIPT=${_TOPLEVEL_COZMO}/project/build-scripts/webots/sdkTest.py

# build the latest clad and SDK
pushd $_TOPLEVEL_COZMO/tools/sdk/cozmoclad
make copy-clad
make dist
popd

#make sure pillow and numpy are installed (for animation test)
$PIP install numpy
$PIP install pillow

#install the pulled down repo and clad
$PIP install -e $_COZMO_REPO_DIR
$PIP install --ignore-installed $_TOPLEVEL_COZMO/tools/sdk/cozmoclad/dist/*.whl

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
