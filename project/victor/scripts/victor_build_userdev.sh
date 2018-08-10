#!/usr/bin/env bash

export GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
${GIT_PROJ_ROOT}/project/victor/scripts/victor_build_release.sh \
                -DANKI_USER_DEV=1 \
                "$@"
