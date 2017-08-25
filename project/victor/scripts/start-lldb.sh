#!/bin/bash

SCRIPT_PATH=`dirname $BASH_SOURCE`
source "${SCRIPT_PATH}/android_env.sh"

# deploy LLDB server

PORT="55001"

APP_ROOT="/data/data/com.anki.cozmoengine"
LLDB_SERVER="${APP_ROOT}/bin/lldb-server"
LLDB_DIST="${HOME}/.anki/android/lldb-server/current"
LLDB_SERVER_SRC="${LLDB_DIST}/lldb-server"

LLDB_SERVER_PID=$($ADB shell pidof lldb-server)
RUNNING=$?

IP_ADDR=$($ADB shell ip addr show wlan0 | grep 'inet ' | cut -d' ' -f6|cut -d/ -f1)
if [ -z $IP_ADDR ]; then
    IP_ADDR=$($ADB shell ip addr show eth0 | grep 'inet ' | cut -d' ' -f6|cut -d/ -f1)
fi

if [ $RUNNING -ne 0 ]; then
    $ADB shell test -f ${LLDB_SERVER}
    if [ $? -ne 0 ]; then
        echo "deploy lldb-server"
        ${SCRIPT_PATH}/fetch-lldb-server.sh
        $ADB push "${LLDB_SERVER_SRC}" "${LLDB_SERVER}"
    fi
    $ADB shell test -f ${APP_ROOT}/lib/lldb-server/libc++_shared.so
    if [ $? -ne 0 ]; then
        echo "deploy lldb-server/libc++_shared.so"
        $ADB shell "mkdir -p ${APP_ROOT}/lib/lldb-server"
        $ADB push "${LLDB_DIST}/libc++_shared.so" "${APP_ROOT}/lib/lldb-server/" 
    fi
    echo "starting lldb-server on port $IP_ADDR:$PORT"
    $ADB shell "cd ${APP_ROOT}/bin && LD_LIBRARY_PATH=${APP_ROOT}/lib/lldb-server nohup ${LLDB_SERVER} platform --server --listen *:$PORT" &
else
    echo "lldb-server running on $IP_ADDR"
fi

echo "stop cozmo engine process"
$ADB shell /data/data/com.anki.cozmoengine/cozmoctl.sh stop

# setup ADB forwarding, if necessary
$ADB forward --list | grep -q "tcp:$PORT tcp:$PORT" || $ADB forward tcp:$PORT tcp:$PORT
