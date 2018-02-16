#!/bin/bash

SCRIPT_PATH=`dirname $BASH_SOURCE`
source "${SCRIPT_PATH}/android_env.sh"

# deploy LLDB server

PORT="55001"

: ${INSTALL_ROOT:="/anki"}

LLDB_SERVER="${INSTALL_ROOT}/bin/lldb-server"
LLDB_DIST="${HOME}/.anki/android/lldb-server/current"
LLDB_SERVER_SRC="${LLDB_DIST}/lldb-server"

RUNNING=0
LLDB_SERVER_PID=$($ADB shell pidof lldb-server)
if [ -z $LLDB_SERVER_PID ]; then
    RUNNING=1
fi

IP_ADDR=$($ADB shell ip addr show wlan0 | grep 'inet ' | cut -d' ' -f6|cut -d/ -f1)
if [ -z $IP_ADDR ]; then
    IP_ADDR=$($ADB shell ip addr show eth0 | grep 'inet ' | cut -d' ' -f6|cut -d/ -f1)
fi

if [ $RUNNING -ne 0 ]; then
    adb_shell "test -f ${LLDB_SERVER}"
    if [ $? -ne 0 ]; then
        echo "deploy lldb-server"
        ${SCRIPT_PATH}/fetch-lldb-server.sh
        $ADB push "${LLDB_SERVER_SRC}" "${LLDB_SERVER}"
    fi
    adb_shell "test -f ${INSTALL_ROOT}/lib/lldb-server/libc++_shared.so"
    if [ $? -ne 0 ]; then
        echo "deploy lldb-server/libc++_shared.so"
        $ADB shell "mkdir -p ${INSTALL_ROOT}/lib/lldb-server"
        $ADB push "${LLDB_DIST}/libc++_shared.so" "${INSTALL_ROOT}/lib/lldb-server/" 
    fi
    echo "starting lldb-server on port $IP_ADDR:$PORT"
    $ADB shell "cd ${INSTALL_ROOT}/bin && LD_LIBRARY_PATH=${INSTALL_ROOT}/lib/lldb-server nohup ${LLDB_SERVER} platform --server --listen *:$PORT" </dev/null >/dev/null 2>&1 &
    if [ $? -eq 0 ]; then
        echo "lldb-server started"
    else
        echo "error: lldb-server failed to start"
        exit 1
    fi
else
    echo "lldb-server running on $IP_ADDR"
fi
