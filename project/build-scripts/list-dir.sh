#!/bin/bash

set -e
set -u

SCRIPT_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))            
_SCRIPT_NAME=`basename ${0}`                                                                        
TOPLEVEL="$( cd "${SCRIPT_PATH}/../.." && pwd )"

function usage() {
    echo "$_SCRIPT_NAME [OPTIONS] <TARGET_DIR>"
    echo "list contents of a directory (recursively)"
    echo "  -h                          print this message"
    echo "  -o OUTPUT                   write output to OUTPUT file"
}

VERBOSE=0

while getopts ":hfvo:" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        v)
            VERBOSE=1
            ;;
        o)
            OUTPUT_FILE=${OPTARG}
            ;;
        :)
            echo "Option -${OPTARG} required an argument." >&2
            usage
            exit 1
            ;;
    esac
done

shift $(expr $OPTIND - 1 )

if [ $# -lt 1 ]; then
    usage
    exit 1
fi

TARGET_DIR="$1"

function LOGv()
{
    if [ $VERBOSE -eq 1 ]; then echo "[$_SCRIPT_NAME] $1"; fi
}

LOGv "List assets"

pushd "$TARGET_DIR" > /dev/null 2>&1

if [ ! -z $OUTPUT_FILE ]; then
  TEMP_OUT=/tmp/list-dir.sh.$$
  find . -not -name '.' | sed -e 's/^\.\///g' | sort > ${TEMP_OUT}
  mv ${TEMP_OUT} ${OUTPUT_FILE}
else
  find . -not -name '.' | sed -e 's/^\.\///g' | sort
fi

popd > /dev/null 2>&1
