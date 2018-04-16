#!/usr/bin/env bash

export GIT_PROJ_ROOT=`git rev-parse --show-toplevel`

if [ $# -eq 1 -a $1 = -h ]; then
    ${GIT_PROJ_ROOT}/project/victor/scripts/deploy.sh "$@"
    exit 0
fi

${GIT_PROJ_ROOT}/project/victor/scripts/stage.sh "$@"

# Also stage vicos
if [ $? -eq 0 ]; then
    PLATFORM_NAME=vicos ${GIT_PROJ_ROOT}/project/victor/scripts/stage.sh -k "$@"
fi

${GIT_PROJ_ROOT}/project/victor/scripts/deploy.sh "$@"
