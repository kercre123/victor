#!/usr/bin/env bash

export GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
${GIT_PROJ_ROOT}/project/victor/scripts/deploy.sh "$@"

# Also deploy vicos
if [ $? -eq 0 ]; then
    PLATFORM_NAME=vicos ${GIT_PROJ_ROOT}/project/victor/scripts/deploy.sh "$@"
fi
