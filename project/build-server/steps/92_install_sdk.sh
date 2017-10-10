#!/bin/bash
#
# Get latest SDK nexclad branch and install it
#

set -e
set -u

PIP=`which pip3`
if [ -z $PIP ];then
  echo pip not found
  exit 1
fi

_SCRIPT_PATH=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)
_TOPLEVEL_COZMO=${_SCRIPT_PATH}/../../..
_COZMO_REPO_DIR=${_TOPLEVEL_COZMO}/tools/sdk/cozmo-python-sdk

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
