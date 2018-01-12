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

if [ -z ${DATA_LOCATION+x} ]; then
    DATA_LOCATION="$HOME/Library/Application Support/Anki/Cozmo"
fi

if [ $# -eq 0 ]
  then
    LANGUAGE=""
  else
  	LANGUAGE="$1"
fi

echo "**********INFO: Installing Scripts and Profile..."
mkdir -p "${DATA_LOCATION}/"
rm -f "${DATA_LOCATION}"/*_Results.log
rm -rf "${DATA_LOCATION}/SaveData.json"
rm -rf "${DATA_LOCATION}/SaveData.json.bak"
echo ${DATA_LOCATION}/
echo cp -rf "${ANKI_REPO_ROOT}/project/build-server/automation-scripts/${LANGUAGE}"/* "${DATA_LOCATION}/"
cp -rf "${ANKI_REPO_ROOT}/project/build-server/automation-scripts/${LANGUAGE}"/* "${DATA_LOCATION}/"


echo "**********INFO: Running command line SmokeTest..."
UNITY_COMMAND="${ANKI_BUILD_UNITY_EXE} -force-free -projectPath ${ANKI_REPO_ROOT}/unity/Cozmo \
-executeMethod Anki.Core.Editor.Automation.CommandLineInterface.PlayAutomationFile  \
-cozmo -smoke -file ${ANKI_REPO_ROOT}/project/build-server/automation-scripts/${LANGUAGE}/SmokeTest.cfg \
-scene Assets/Scenes/Bootstrap.unity"

expect -c "set timeout ${UNITY_TIMEOUT}; spawn -noecho ${UNITY_COMMAND}; expect timeout { exit 1 } eof { exit 0 }"

echo "**********INFO: Grabbing Results..."
mkdir -p "${ANKI_REPO_ROOT}/build/automation/"
cp "${DATA_LOCATION}"/*_Results.log "${ANKI_REPO_ROOT}/build/automation/"

# Force a good status until functionality has been vetted


exit $?
