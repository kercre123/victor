#!/bin/bash
set -xe

echo "=== Test Sdk ==="

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
source "${SCRIPT_PATH}/../build_env.sh"

APP_BUNDLE="${BUILT_PRODUCTS_PATH}/${ANKI_BUILD_BUNDLE_NAME}.app"

make -C ${ANKI_SDK_ROOT}/cozmoclad copy-clad
pip3 install -e tools/sdk/cozmoclad/
#install to unit and run it
${ANKI_BUILD_TOOLS_ROOT}/testSdk.py --sdk-root ${ANKI_SDK_ROOT} --sdk-dev-root ${ANKI_SDK_DEV_ROOT} --app-artifact ${APP_BUNDLE}

if [ ! -d das_logs ]; then
    echo "\nError: Logs not created.\n"
    exit 1
fi
tar -czvf das_logs.tgz das_logs
