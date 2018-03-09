#!/usr/bin/env bash

GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
${GIT_PROJ_ROOT}/project/victor/scripts/victor_stop.sh &&\
${GIT_PROJ_ROOT}/project/victor/scripts/deploy.sh &&\
${GIT_PROJ_ROOT}/project/victor/scripts/victor_restart.sh
