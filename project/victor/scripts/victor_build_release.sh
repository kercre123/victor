#!/usr/bin/env bash

export GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
${GIT_PROJ_ROOT}/project/victor/build-victor.sh \
        -D ANKI_BUILD_OPENCL_ION_MEM_EXAMPLE=1 \
		-c Release \
		"$@"
