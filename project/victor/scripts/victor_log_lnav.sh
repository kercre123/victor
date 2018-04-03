#!/bin/bash

set -e
set -u

# Check that lnav is installed
if [ ! -x "$(command -v lnav)" ]; then
  echo "ERROR: lnav not found. Do \"brew install lnav\""
  exit 1
fi

# Get the directory of this script                                                                    
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=`basename ${0}`
GIT=`which git`
if [ -z $GIT ]
then
    echo git not found
    exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

# defaults
LNAV_PATH=${SCRIPT_PATH}/lnav
LOG_FILE=${LNAV_PATH}/victor_log_lnav.log
LNAV_FILTER_FILE=${LNAV_PATH}/lnav_filters.txt
LNAV_FORMAT_FILE=${LNAV_PATH}/victor_log.json
LNAV_FORMAT_FILE_INSTALL_LOC=~/.lnav/formats/installed/victor_log.json
UNINSTALL_FORMAT_FILE=0

function usage() {
    echo "$SCRIPT_NAME [OPTIONS]"
    echo "  -h                      Print this message"
    echo "  -o <log file>           File to log unfiltered data to (Default: ${LOG_FILE})"
    echo "  -f <filter file>        lnav filter file to use (Default: ${LNAV_FILTER_FILE})"
    echo "  -u                      Display without formatting (i.e. uninstalls lnav format file)"
    echo "  -s                      hostname or ip address of robot (Default: ${ANKI_ROBOT_HOST})"
}

while getopts ":o:f:uhs:" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        o)
            LOG_FILE="${OPTARG}"
            ;;
        f)
            LNAV_FILTER_FILE="${OPTARG}"
            ;;
        u)
            UNINSTALL_FORMAT_FILE=1
            ;;            
        s)
            ANKI_ROBOT_HOST="${OPTARG}"
            ;;
        :)
            echo "Option -${OPTARG} required an argument." >&2
            usage
            exit 1
            ;;
    esac
done                   

echo "LNAV_FILTER_FILE: ${LNAV_FILTER_FILE}"
echo "LOG_FILE: ${LOG_FILE}"


source ${SCRIPT_PATH}/victor_env.sh

# Define ANKI_ROBOT_HOST
robot_set_host

# Delete the log file if it exists
if [ -f "$LOG_FILE" ]; then
  rm ${LOG_FILE}
fi

# Calling full ssh logcat command here. 
# Doing it via robot_sh makes $! return the wrong pid for some reason.
ssh ${ANKI_ROBOT_USER}@${ANKI_ROBOT_HOST} logcat > ${LOG_FILE} &

# Get PID so we can kill the backgrounded task at the end
LOGGING_PID=$!
echo "Logging to file ${LOG_FILE} (PID: {$LOGGING_PID})"

# Make sure we kill LOGGING_PID no matter how lnav exits!
set +e

# If lnav format file is already installed and we don't want it
# to be, then uninstall it
if [ ${UNINSTALL_FORMAT_FILE} -eq 1 ] && [ -e "${LNAV_FORMAT_FILE_INSTALL_LOC}" ]; then
  echo "Uninstalling lnav format: ${LNAV_FORMAT_FILE_INSTALL_LOC}"
  rm ${LNAV_FORMAT_FILE_INSTALL_LOC}
fi

# Install lnav format file in case it isn't already
if [ ${UNINSTALL_FORMAT_FILE} -eq 0 ] && [ -s "${LNAV_FORMAT_FILE}" ]; then
  echo "Installing lnav format: ${LNAV_FORMAT_FILE}"
  lnav -i ${LNAV_FORMAT_FILE}
fi  

# Start lnav
if [ -s "${LNAV_FILTER_FILE}" ]; then
  echo "Starting lnav on ${LOG_FILE} with filter file ${LNAV_FILTER_FILE}..."
  lnav -f ${LNAV_FILTER_FILE} ${LOG_FILE}
else 
  echo "Starting lnav on ${LOG_FILE}..."
  lnav ${LOG_FILE}
fi

echo "Quitting lnav..." 
kill -9 ${LOGGING_PID}