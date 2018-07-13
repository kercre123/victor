#!/bin/bash
set -e

: ${ANKI_AUTH_TOKEN:=""}
: ${TEAMCITY_BUILD_BRANCH:=""}

echo "=== SWITCH TO MERGE BRANCH ==="

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
source "${SCRIPT_PATH}/../build_env.sh"

if [ ! -z ${TEAMCITY_BUILD_BRANCH+x} ] && [ ! -z ${ANKI_AUTH_TOKEN+x} ]; then
  ./project/buildScripts/pr_mergeablity.py -v -b ${TEAMCITY_BUILD_BRANCH} -a ${ANKI_AUTH_TOKEN} -r "anki/victor"
  exit $?
else
  echo "Warning: This script should only be run through a CI system."
  exit 1
fi
