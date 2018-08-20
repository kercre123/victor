#!/bin/bash

# Properly build Anki Maya Wwise Plugin.
# Add environment variables to include plugin CMake project
# Build release version of plugin

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=`basename ${0}`
TOPLEVEL=$(cd "${SCRIPT_PATH}/../../.." && pwd)

echo "$SCRIPT_NAME - Run victor build script to setup workspace and build maya plugin!"

${TOPLEVEL}/project/victor/build-victor.sh -f -S -p mac -c Release -t anki_maya_wwise_plugin -a -DBUILD_MAYA_WWISE_PLUGIN=ON

if [ $? -ne 0 ]; then
  echo "Failed to build Maya Wwise plugin!"
  exit 1
fi

# Print location of plugin .bundle
PLUGIN_BUILD_PATH="${TOPLEVEL}/_build/mac/Release/lib/AnkiMayaWWisePlugIn.bundle"
echo "Maya Wwise Plugin build success!!"
echo "Plugin is located at '${PLUGIN_BUILD_PATH}'"

exit 0
