#!/bin/bash
#
# Upload string files to Smartling
#
# See https://help.smartling.com/v1.0/docs/file-support for info about
# the supported file types/formats
#
# This script looks for two environment variables:
# (1) SMARTLING_PUSH_SEARCH_DIR - This specifies the directory to search for
#     files to upload to Smartling. If this variable isn't specified (or is an
#     empty string), the entire Git repo is searched. If a relative directory
#     is specified, it is relative to the root of the Git repo.
# (2) SMARTLING_PUSH_SEARCH_PATTERN - This specifies the pattern to search for
#     files to upload to Smartling (see the "-path" flag for the "find" command
#     for some related info). This pattern is "*en-US/*.json" for Cozmo
#     and "*base/*-strings.json" for OverDrive, but the default is "*.json".


set -e

# Check for the optional environment variables before "set -u"

if [ -n "${SMARTLING_PUSH_SEARCH_DIR}" ]; then
  SEARCH_DIR=${SMARTLING_PUSH_SEARCH_DIR}
else
  SEARCH_DIR=""
fi

if [ -n "${SMARTLING_PUSH_SEARCH_PATTERN}" ]; then
  SEARCH_PATTERN=${SMARTLING_PUSH_SEARCH_PATTERN}
else
  SEARCH_PATTERN="*.json"
fi

set -u

VERBOSE=1
function logv()
{
  [ $VERBOSE -ne 0 ] && echo "$1"
}

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`
cd $TOPLEVEL

_SCRIPT_PATH=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)                                              
SMARTLING_HOME="${_SCRIPT_PATH}/bash"

if [ -z $SEARCH_DIR ]; then
  # If no search directory was specified via $SMARTLING_PUSH_SEARCH_DIR
  # above, then search the entire Git repo.
  SEARCH_DIR=$TOPLEVEL
else
  if [[ $SEARCH_DIR != /* ]]; then
    # If the search directory is not absolute, it is relative to
    # the root of the Git repo.
    SEARCH_DIR="${TOPLEVEL}/${SEARCH_DIR}"
  fi
fi

find ${SEARCH_DIR} -path "${SEARCH_PATTERN}" -print0 | while read -d $'\0' file

do
  logv "upload strings file: ${file}" 
  ${SMARTLING_HOME}/upload.sh -t json ${file}
done

