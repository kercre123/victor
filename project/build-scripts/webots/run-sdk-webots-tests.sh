#!/bin/bash
#
# Get latest SDK nexclad branch, try to run SDK tests against cozmo-one master code with Webots
#

set -e
set -u

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi

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
_COZMO_REPO_DIR=cozmo-python-sdk
_GIT_COZMO_SDK_URI=https://git@github.com/anki/${_COZMO_REPO_DIR}.git
_GIT_BRANCH_NAME=nextclad
_GIT_USERNAME="anki-smartling"
_GIT_EMAIL="anki-smartling@anki.com"
_SDK_WEBOTS_SCRIPT=${_TOPLEVEL_COZMO}/project/build-scripts/webots/sdkTest.py
_SDK_SCRIPT_LOCATION=${_TOPLEVEL_COZMO}/${_COZMO_REPO_DIR}/examples

# clone cozmo-python-sdk
if [ -d $_COZMO_REPO_DIR ]; then
    pushd $_COZMO_REPO_DIR
    $GIT fetch -p
    $GIT checkout master
    # delete any existing local branches other than master
    if [ $($GIT branch | wc -l) != 1 ]; then
        $GIT branch | grep -v '* master' | xargs git branch -D
    fi
    $GIT checkout $_GIT_BRANCH_NAME
    popd
else
    $GIT clone $_GIT_COZMO_SDK_URI
    pushd $_COZMO_REPO_DIR
    $GIT checkout $_GIT_BRANCH_NAME
    popd
fi

# install latest clad and SDK with pip
pushd $_TOPLEVEL_COZMO/tools/sdk/cozmoclad
make copy-clad
make dist
$PIP install --ignore-installed $_TOPLEVEL_COZMO/$_COZMO_REPO_DIR
$PIP install --ignore-installed $_TOPLEVEL_COZMO/tools/sdk/cozmoclad/dist/*.whl
popd

NUM_RUNS_PARAM=""
if [[ ${NUM_RUNS:-} ]];then
  NUM_RUNS_PARAM="--numRuns $NUM_RUNS"
fi
SDK_TIMEOUT_PARAM=""
if [[ ${SDK_TIMEOUT:-} ]];then
  SDK_TIMEOUT_PARAM="--timeout $SDK_TIMEOUT"
fi
$PYTHON $_SDK_WEBOTS_SCRIPT --sdkScriptLocation ${_SDK_SCRIPT_LOCATION} $SDK_TIMEOUT_PARAM $NUM_RUNS_PARAM
exit $?
