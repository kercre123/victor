#!/bin/bash
set -e
set -u

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
source "${SCRIPT_PATH}/../build_env.sh"

if [ -z ${ANKI_BUILD_UNITY_EXE+x} ]; then
  ANKI_BUILD_UNITY_EXE=/Applications/Unity/Unity.app/Contents/MacOS/Unity
fi

if [ -z ${UNITY_TIMEOUT+x} ]; then
    UNITY_TIMEOUT=12000
fi

if [ -n "${UNITY_USER+x}" ]; then
  echo "**********INFO: Unregistering Unity..."
  UNITY_COMMAND="${ANKI_BUILD_UNITY_EXE} -nographics -batchmode -quit -returnlicense \
  -logFile ${ANKI_REPO_ROOT}/build/automation/UnityBuild-Unregister.log"

  echo ${UNITY_COMMAND}
  expect -c "set timeout ${UNITY_TIMEOUT}; spawn -noecho ${UNITY_COMMAND}; expect timeout { exit 1 } eof { exit 0 }"

  echo "**********INFO: Registering Unity..."
  UNITY_COMMAND="${ANKI_BUILD_UNITY_EXE} -nographics -batchmode -quit \
  -serial ${UNITY_SERIAL} -username \"${UNITY_USER}\" -password \"${UNITY_PASSWORD}\" \
  -logFile ${ANKI_REPO_ROOT}/build/automation/UnityBuild-Register.log"

  echo ${UNITY_COMMAND}
  expect -c "set timeout ${UNITY_TIMEOUT}; spawn -noecho ${UNITY_COMMAND}; expect timeout { exit 1 } eof { exit 0 }"
fi
