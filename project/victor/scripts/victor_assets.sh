#!/usr/bin/env bash

GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
${GIT_PROJ_ROOT}/project/victor/scripts/deploy-assets.sh ${GIT_PROJ_ROOT}/_build/android/Release/assets
