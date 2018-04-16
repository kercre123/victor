#!/bin/bash

# Stage android and vicos
./project/victor/scripts/stage.sh -c Release
PLATFORM_NAME=vicos ${GIT_PROJ_ROOT}/project/victor/scripts/stage.sh -k -c Release

ANKI_BUILD_SHA=`git rev-parse --short HEAD`
DEPLOYABLES_FILE="deployables_${ANKI_BUILD_SHA}.tar.gz"

tar -pczf "_build/${DEPLOYABLES_FILE}" -C _build/staging Release \
                                       -C ../../tools/ rsync \
                                       -C ../project/victor/scripts victor_env.sh deploy.sh deploy_build_artifacts.sh
