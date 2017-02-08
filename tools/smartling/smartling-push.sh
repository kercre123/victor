#!/bin/bash
#
# Upload strings files to smartling
#

set -e
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

find ${TOPLEVEL} -path "*en-US/*.json" -print0 | while read -d $'\0' file
do
  logv "upload strings file: ${file}" 
  ${SMARTLING_HOME}/upload.sh -t json ${file} 
done

