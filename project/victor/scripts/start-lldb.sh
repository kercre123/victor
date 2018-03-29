#!/bin/bash

# Get the directory of this script
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=$(basename ${0})
GIT=`which git`
if [ -z $GIT ]
then
    echo git not found
    exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
source ${GIT_PROJ_ROOT}/project/victor/scripts/victor_env.sh

source ${GIT_PROJ_ROOT}/project/victor/scripts/host_robot_ip_override.sh

robot_set_host

# deploy LLDB server

PORT="55001"

: ${INSTALL_ROOT:="/anki"}

LLDB_SERVER="${INSTALL_ROOT}/bin/lldb-server"
LLDB_DIST="${HOME}/.anki/android/lldb-server/current"
LLDB_SERVER_SRC="${LLDB_DIST}/lldb-server"

RUNNING=0
LLDB_SERVER_PID=$(robot_sh pidof lldb-server)
if [ -z $LLDB_SERVER_PID ]; then
    RUNNING=1
fi

if [ $RUNNING -ne 0 ]; then
    robot_sh "test -f ${LLDB_SERVER}"
    if [ $? -ne 0 ]; then
        echo "deploy lldb-server"
        ${SCRIPT_PATH}/fetch-lldb-server.sh
        robot_cp "${LLDB_SERVER_SRC}" "${LLDB_SERVER}"
    fi
    robot_sh "test -f ${INSTALL_ROOT}/lib/lldb-server/libc++_shared.so"
    if [ $? -ne 0 ]; then
        echo "deploy lldb-server/libc++_shared.so"
        robot_sh "mkdir -p ${INSTALL_ROOT}/lib/lldb-server"
        robot_cp "${LLDB_DIST}/libc++_shared.so" "${INSTALL_ROOT}/lib/lldb-server/" 
    fi
    echo "starting lldb-server on $ANKI_ROBOT_HOST:$PORT"
    robot_sh "cd ${INSTALL_ROOT}/bin && LD_LIBRARY_PATH=${INSTALL_ROOT}/lib/lldb-server nohup ${LLDB_SERVER} platform --server --listen *:$PORT" </dev/null >/dev/null 2>&1 &
    if [ $? -eq 0 ]; then
        echo "lldb-server started"
    else
        echo "error: lldb-server failed to start"
        exit 1
    fi
else
    echo "lldb-server running on $ANKI_ROBOT_HOST:$PORT"
fi
