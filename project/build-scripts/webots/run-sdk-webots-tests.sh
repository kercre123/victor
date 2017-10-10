#!/bin/bash
#
# Convience script which will run two steps for automation
#

set -e
set -u

_SCRIPT_PATH=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)
_TOPLEVEL_COZMO=${_SCRIPT_PATH}/../../..

$_TOPLEVEL_COZMO/project/build-server/steps/92_install_sdk.sh
$_TOPLEVEL_COZMO/project/build-server/steps/95_run_sdk_webots_tests.sh
exit $?
