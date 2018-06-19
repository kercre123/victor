#!/bin/bash

set -u

# Go to directory of this script
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=$(basename ${0})
GIT=`which git`
if [ -z $GIT ]
then
    echo git not found
    exit 1
fi
: ${TOPLEVEL:=`$GIT rev-parse --show-toplevel`}
LOCAL_CRASH_FOLDER=${SCRIPT_PATH}/crashes

set -e

source ${TOPLEVEL}/project/victor/scripts/victor_env.sh

# Settings can be overridden through environment
: ${ANKI_BUILD_TYPE:="Release"}

: ${OUTPUT_STACK_CONTENTS:="0"}
: ${CALLSTACK_WITH_SYMBOLS:="1"}
: ${PULL_CRASH_FROM_ROBOT:="0"}
: ${VERBOSE:="0"}
: ${CRASH_DUMP_FILE:=""}
: ${VIC_CRASH_FOLDER:="/data/data/com.anki.victor/cache/crashDumps"}

# TODO To be replaced with a shared VM:
: ${REMOTE_HOST:='pterry@10.10.7.148'}
: ${REMOTE_DEST_FOLDER_BASE:='/home/pterry/test'}

# To allow multiple simultaneous users of this tool (with
# the same VM), create a subfolder unique to them
REMOTE_DEST_FOLDER=${REMOTE_DEST_FOLDER_BASE}/${USER}

function usage() {
  echo "$SCRIPT_NAME [OPTIONS]"
  echo "options:"
  echo "  -h                  print this message"
  echo "  -c ANKI_BUILD_TYPE  build configuration {Debug,Release}"
  echo "  -k                  output stack contents"
  echo "  -n                  no symbols (just generate a non-symbolicated callstack)"
  echo "  -r                  pull crash dump file from robot (otherwise, find locally)"
  echo "  -s ANKI_ROBOT_HOST  hostname or ip address of robot"
  echo "  -x <crashDumpFile>  filename or path/filename for the crash dump file"
  echo "  -v                  verbose"
  echo ""
  echo "environment variables:"
  echo '  $ANKI_ROBOT_HOST    hostname or ip address of robot'
  echo '  $ANKI_BUILD_TYPE    build configuration {Debug,Release}'
  echo ""
  echo "If you use option -r and don't provide a crashDumpFile, the most recent crash dump will be pulled from robot"
}

while getopts "hc:knrs:x:v" opt; do
  case $opt in
    h)
      usage && exit 0
      ;;
    c)
      ANKI_BUILD_TYPE="${OPTARG}"
      ;;
    k)
      OUTPUT_STACK_CONTENTS="1"
      ;;
    n)
      CALLSTACK_WITH_SYMBOLS="0"
      ;;
    r)
      PULL_CRASH_FROM_ROBOT="1"
      ;;
    s)
      ANKI_ROBOT_HOST="${OPTARG}"
      ;;
    x)
      CRASH_DUMP_FILE="${OPTARG}"
      ;;
    v)
      VERBOSE="1"
      ;;
    *)
      usage && exit 1
      ;;
  esac
done

if [ -z ${CRASH_DUMP_FILE} ]; then
  if [ $PULL_CRASH_FROM_ROBOT -eq "0" ]; then
    echo "No crash dump filename provided; you must specify a crash dump file (with -x) unless using -r to pull from robot (in which case it's optional)"
    usage
    exit 1
  else
    echo "No crash dump filename provided...will pull most recent crash from robot"
  fi
fi

robot_set_host

echo "ANKI_BUILD_TYPE: ${ANKI_BUILD_TYPE}"
if [ $PULL_CRASH_FROM_ROBOT -eq "1" ]; then
  echo "ANKI_ROBOT_HOST: ${ANKI_ROBOT_HOST}"
fi

mkdir -p ${LOCAL_CRASH_FOLDER}

# Pull crash dump file off of robot if requested; otherwise find it locally

if [ $PULL_CRASH_FROM_ROBOT -eq "1" ]; then
  if [ -z ${CRASH_DUMP_FILE} ]; then
    echo "Searching for latest crash dump file on robot at "$ANKI_ROBOT_HOST
    # List all regular (non directory) files whose size is greater than zero, and sort by time
    cmd="cd ${VIC_CRASH_FOLDER}; find . -type f -size +0c -exec ls -t {} + -o -name . -o -prune | head -1"
    CRASH_DUMP_FILE="$(ssh root@${ANKI_ROBOT_HOST} $cmd)"
    if [ -z ${CRASH_DUMP_FILE} ]; then
      echo "No valid crash dump file found on robot"
      exit 1
    fi
  fi
  scp -p root@${ANKI_ROBOT_HOST}:${VIC_CRASH_FOLDER}/${CRASH_DUMP_FILE} ${LOCAL_CRASH_FOLDER}/
else
  ### TODO: See if $CRASH_DUMP_FILE has a path in it; if so, look for that rather than in local_crash_folder
  ### ...keep the base name separate, for use on the VM later when running minidump_stackwalk...
  if [ ! -f ${LOCAL_CRASH_FOLDER}/${CRASH_DUMP_FILE} ]; then
    echo "Crash dump file ${CRASH_DUMP_FILE} not found in folder ${LOCAL_CRASH_FOLDER}"
    exit 1
  fi
fi

ssh $REMOTE_HOST "mkdir -p ${REMOTE_DEST_FOLDER}"

# Ensure we have the two linux tools (that need to run on the VM)
# up on that VM (in the base folder, not the folder for this run)
echo "Ensuring tools are on VM..."
rsync -lptu ${TOPLEVEL}/tools/crash-tools/linux/dump_syms $REMOTE_HOST:$REMOTE_DEST_FOLDER_BASE/
rsync -lptu ${TOPLEVEL}/tools/crash-tools/linux/minidump_stackwalk $REMOTE_HOST:$REMOTE_DEST_FOLDER_BASE/

CALLSTACK_FILENAME=callstack.txt
ssh $REMOTE_HOST rm -f ${REMOTE_DEST_FOLDER}/${CALLSTACK_FILENAME}
rm -f ${CALLSTACK_FILENAME}

if [ $CALLSTACK_WITH_SYMBOLS -eq "1" ]; then
  echo "Creating symbols from local build..."

  if [ $VERBOSE -eq "0" ]; then
    SCP_QUIET="-q"
  else
    SCP_QUIET=""
  fi  

  SRC_FOLDER=${TOPLEVEL}/_build/vicos/${ANKI_BUILD_TYPE}
  ssh $REMOTE_HOST "rm -rf $REMOTE_DEST_FOLDER/lib"
  ssh $REMOTE_HOST "rm -rf $REMOTE_DEST_FOLDER/bin"
  ssh $REMOTE_HOST "mkdir $REMOTE_DEST_FOLDER/lib"
  ssh $REMOTE_HOST "mkdir $REMOTE_DEST_FOLDER/bin"
  scp $SCP_QUIET $SRC_FOLDER/lib/*.full $REMOTE_HOST:$REMOTE_DEST_FOLDER/lib/
  scp $SCP_QUIET $SRC_FOLDER/bin/*.full $REMOTE_HOST:$REMOTE_DEST_FOLDER/bin/

  # Run commands on remote VM; these do the symbol package creation
  TEMP_CMD_FILE=get-callstack-temp
  rm -f ${TEMP_CMD_FILE}
  echo "cd ${REMOTE_DEST_FOLDER}" > ${TEMP_CMD_FILE}
  cat ${TOPLEVEL}/tools/crash-tools/get-callstack-remote-cmds >> ${TEMP_CMD_FILE}
  if [ $VERBOSE -eq "1" ]; then
    cat ${TEMP_CMD_FILE} | ssh $REMOTE_HOST
  else
    cat ${TEMP_CMD_FILE} | ssh $REMOTE_HOST > /dev/null
  fi
  rm ${TEMP_CMD_FILE}
fi

echo "Generating callstack..."
scp -p ${LOCAL_CRASH_FOLDER}/${CRASH_DUMP_FILE} $REMOTE_HOST:${REMOTE_DEST_FOLDER}/
cmd="cd ${REMOTE_DEST_FOLDER}; ../minidump_stackwalk ${CRASH_DUMP_FILE}"
if [ $CALLSTACK_WITH_SYMBOLS -eq "1" ]; then
  cmd="$cmd symbols/"
fi
if [ $OUTPUT_STACK_CONTENTS -eq "1" ]; then
  cmd="$cmd -s"
fi 
cmd="$cmd 2> /dev/null > $CALLSTACK_FILENAME"
ssh $REMOTE_HOST $cmd
scp $REMOTE_HOST:${REMOTE_DEST_FOLDER}/${CALLSTACK_FILENAME} .
echo ""
echo ${CALLSTACK_FILENAME}" written to "$PWD
