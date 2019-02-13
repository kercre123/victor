#!/usr/bin/env bash

export GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
${GIT_PROJ_ROOT}/project/victor/build-victor.sh \
		-c Release \
		-DALEXA_ACOUSTIC_TEST=1 \
		-DANKI_AMAZON_ENDPOINTS_ENABLED=1 \
		"$@"
