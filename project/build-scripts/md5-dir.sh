#!/bin/bash

set -e
set -u

SCRIPT_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
#SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))            
_SCRIPT_NAME=`basename ${0}`                                                                        
TOPLEVEL="$( cd "${SCRIPT_PATH}/../.." && pwd )"

function usage() {
    echo "$_SCRIPT_NAME [OPTIONS] <TARGET_DIR>"
    echo "calculate the md5 digest of the entire contents of a directory (recursively)"
    echo "  -h                          print this message"
    echo "  -f                          force assets to be copied"
    echo "  -o OUTPUT                   write hash to OUTPUT file"
}

FORCE=0
VERBOSE=0

while getopts ":hfvo:" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        f)
            FORCE=1
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

LOGv "Calculate hash digest of all assets"

SESSION_KEY="$$"
RESOURCES_FILE="/tmp/resources.${SESSION_KEY}.txt"
HASHES_FILE="/tmp/resources.hash_${SESSION_KEY}.txt"

pushd "$TARGET_DIR" > /dev/null 2>&1

find . -not -name '.' -print | sed -e 's/^\.\///g' > $RESOURCES_FILE

IFS=$'\n'       # make newlines the only separator
set -f          # disable globbing
MD5_FILELIST=()
for i in $(cat < "$RESOURCES_FILE"); do
  if [ -f "$i" ]; then
    MD5_FILELIST+=("$i")
  fi
done

# calculate hashes of for each file in the list
md5 -q "${MD5_FILELIST[@]}" > "${HASHES_FILE}"

# combine all filenames and hashes
CONTENTS_FILE="/tmp/resources.contentshash_${SESSION_KEY}.txt"
tr -d '\n' < ${RESOURCES_FILE} >> ${CONTENTS_FILE}
tr -d '\n' < ${HASHES_FILE} >> ${CONTENTS_FILE}

CONTENTS_HASH=$(md5 -q "${CONTENTS_FILE}")

if [ ! -z $OUTPUT_FILE ]; then
    echo "${CONTENTS_HASH}" > $OUTPUT_FILE
else
    echo "${CONTENTS_HASH}"
fi

rm -f "${RESOURCES_FILE}"
rm -f "${HASHES_FILE}"
rm -f "${CONTENTS_FILE}"

popd > /dev/null 2>&1
