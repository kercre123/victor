#!/system/bin/sh

set -u
set -e

CCMD="$1"
ARGV="$@"

SCRIPT_NAME=$(basename $(readlink -f $0))
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))

BIN_PATH="${SCRIPT_PATH}"
LIB_PATH="${SCRIPT_PATH}"

#
# Define required functions & params
#

# check for binaries

# executable name
PROGRAM_EXE="boinc"

# human-friendly display name
PROGRAM_NAME="boinc"

if [ ! -f ${BIN_PATH}/${PROGRAM_EXE} ]; then
    echo "${PROGRAM_NAME} not found at ${BIN_PATH}/${PROGRAM_EXE}"
    exit 1
fi

source ${SCRIPT_PATH}/ankienv.sh

set +e

function start_program()
{
    PROG_PID=$(pidof ${PROGRAM_EXE})
    PROG_RUNNING=$?

    if [ $PROG_RUNNING -eq 0 ]; then
        echo "${PROGRAM_NAME} already running. Use stop or restart"
        exit 1
    fi

    export LD_LIBRARY_PATH="${LIB_PATH}"

    cd ${SCRIPT_PATH}
    exec_background ${BIN_PATH}/${PROGRAM_EXE}
    PROG_PID=$!
    echo "started ${PROGRAM_EXE} (${PROG_PID})"
    taskset -a -p 8 ${PROG_PID}
    echo ${PROG_PID}
}

function stop_program()
{
    stop_process ${PROGRAM_EXE}
}

function program_status()
{
    process_status ${PROGRAM_EXE}
}

main $*
