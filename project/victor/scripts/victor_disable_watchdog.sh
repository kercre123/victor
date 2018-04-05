#!/usr/bin/env bash
#
set -eu

GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
systemctl=${GIT_PROJ_ROOT}/project/victor/scripts/systemctl.sh
svc=vic-watchdog.service

${systemctl} disable --now ${svc}

