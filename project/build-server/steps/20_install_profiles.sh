#!/bin/bash

set -e
set -u

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
if [ -f ${SCRIPT_PATH}/../build_env.sh ]; then
  source ${SCRIPT_PATH}/../build_env.sh
fi

pushdir ${ANKI_REPO_ROOT}

# remove installed profiles
${ANKI_BUILD_TOOLS_ROOT}/mputil.py remove ./project/ios/ProvisioningProfiles/*.mobileprovision

# install profiles
${ANKI_BUILD_TOOLS_ROOT}/mputil.py install ./project/ios/ProvisioningProfiles/*.mobileprovision

popdir
