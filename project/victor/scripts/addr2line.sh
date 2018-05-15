#!/bin/bash

if [ "$#" == "0" ]; then
  echo "Usage: $0 <name of .so> <address> [<address>]"
  exit 1
fi

GIT_PROJ_ROOT=`git rev-parse --show-toplevel`

ADDR2LINE=`${GIT_PROJ_ROOT}/project/victor/scripts/vicos_which.sh addr2line`

configuration=Release
shared_library=$1
shift

${ADDR2LINE} -e ${GIT_PROJ_ROOT}/_build/vicos/${configuration}/lib/${shared_library}.full -a $@
