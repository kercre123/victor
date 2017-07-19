#!/bin/bash
set -e
set -u

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
source "${SCRIPT_PATH}/../build_env.sh"
ANKI_BUILD_UNITY_EXE=/Applications/Unity/Unity.app/Contents/MacOS/Unity

if [ -z ${UNITY_TIMEOUT+x} ]; then
    UNITY_TIMEOUT=6000
fi


echo "**********INFO: Installing Scripts and Profile..."
rm -rf ~/Library/Application\ Support/Anki/Cozmo/*_Results.log
cp -rf ${ANKI_REPO_ROOT}/project/build-server/automation-scripts/* ~/Library/Application\ Support/Anki/Cozmo/


echo "**********INFO: Running command line SmokeTest..."
UNITY_COMMAND="${ANKI_BUILD_UNITY_EXE} -force-free -projectPath ${ANKI_REPO_ROOT}/unity/Cozmo \
-executeMethod Anki.Core.Editor.Automation.CommandLineInterface.PlayAutomationFile  \
-cozmo -smoke -file ${ANKI_REPO_ROOT}/project/build-server/automation-scripts/SmokeTest.cfg \
-scene Assets/Scenes/Bootstrap.unity"

expect -c "set timeout ${UNITY_TIMEOUT}; spawn -noecho ${UNITY_COMMAND}; expect timeout { exit 1 } eof { exit 0 }"

echo "**********INFO: Grabbing Results..."
mkdir ${ANKI_REPO_ROOT}/build/automation/ 2> /dev/null
cp ~/Library/Application\ Support/Anki/Cozmo/*_Results.log ${ANKI_REPO_ROOT}/build/automation/


exit $?
