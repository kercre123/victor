#!/system/bin/sh

set -u
set -e

CCMD="$1"
ARGV="$@"

# check for android
VICTOR_ANDROID=0

MACHINE=$(uname -m)
if [ "${MACHINE}" == "armv7l" ]; then
    VICTOR_ANDROID=1
fi

if [ $VICTOR_ANDROID -ne 1 ]; then
    echo "This program must be run on Android OS"
    exit 1
fi

SCRIPT_NAME=$(basename $(readlink -f $0))
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))

# check for libs
LIB_PATH="${SCRIPT_PATH}/lib"

if [ ! -d ${LIB_PATH} ]; then
    echo "Required libraries not found in ${LIB_PATH}"
    exit 1
fi

#
# Define required functions & params
#

# check for binaries
PROGRAM_EXE="cloud_process"
PROGRAM_NAME="cloud_process"

BIN_PATH="${SCRIPT_PATH}/bin"
if [ ! -f ${BIN_PATH}/${PROGRAM_EXE} ]; then
    echo "cloud process not found at ${BIN_PATH}/${PROGRAM_EXE}"
    exit 1
fi

source ${SCRIPT_PATH}/ankienv.sh

set +e


function start_program()
{
    PROG_PID=$(pidof ${PROGRAM_EXE})
    PROG_RUNNING=$?

    if [ $PROG_RUNNING -eq 0 ]; then
        echo "Engine already running. Use stop or restart"
        exit 1
    fi

    export LD_LIBRARY_PATH="${LIB_PATH}"

    # start engine
    cd ${SCRIPT_PATH}
    exec_background ${BIN_PATH}/${PROGRAM_EXE}
    echo $!
}

function stop_program()
{
    stop_process ${PROGRAM_EXE}
}

function program_status()
{
    process_status ${PROGRAM_EXE}
}

function get_program_pid()
{
    get_process_pid ${PROGRAM_EXE}
}

function get_program_name()
{
    get_process_name ${PROGRAM_EXE}
}

main $*
