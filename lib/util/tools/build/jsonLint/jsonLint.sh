#!/bin/bash

set -e
set -u

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
JSONLINT=${SCRIPT_PATH}/demjson/jsonlint 

errors_found=false
for file in $(find $1 -name '*.json'); do
  exit_status=0
  out=$($JSONLINT --allow comments,non-portable $file) || exit_status=$?

  if [ $exit_status -ne 0 ]; then
    echo $out
    errors_found=true
  fi
done

if $errors_found ; then
  echo "FAILED: JSON errors found!"
  exit 1
else
  exit 0
fi
