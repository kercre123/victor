#!/usr/bin/env bash

export GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
${GIT_PROJ_ROOT}/project/victor/build-victor.sh \
                -c Release \
                -DANKI_PRIVACY_GUARD=1 \
                -DANKI_NO_WEBSERVER_ENABLED=1 \
                -DANKI_DEV_CHEATS=0 \
                -DANKI_PROFILING_ENABLED=0 \
                -DANKI_REPORT_ERRORS_TO_DAS=0 \
                -DANKI_REPORT_ERRORS_WITH_STRVAL_TO_DAS=0 \
                -DANKI_MESSAGE_PROFILER_ENABLED=0 \
                -DANKI_BREADCRUMBS=0 \
                -DANKI_RESOURCE_SHIPPING=1 \
                -DANKI_BUILD_OPENCL_ION_MEM_EXAMPLE=0 \
                -DREMOTE_CONSOLE_ENABLED=0 \
                -DUSE_ANKITRACE=OFF\
                -a -DAUDIO_RELEASE=ON \
                "$@"
