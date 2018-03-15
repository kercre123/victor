#!/usr/bin/env bash
#
set -eu

GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
systemctl=${GIT_PROJ_ROOT}/project/victor/scripts/systemctl.sh
svcfn=/anki/etc/systemd/system/vic-logmgr.service
svc=$(basename ${svcfn})

${systemctl} enable --now ${svcfn}

