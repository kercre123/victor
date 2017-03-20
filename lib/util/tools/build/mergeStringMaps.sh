#!/bin/bash
# Copyright (c) 2014 Anki, Inc.
# All rights reserved.

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
source "${SCRIPT_PATH}/build_env.sh"

profile_begin "" "[$@]"

function usage() {
  echo "$0 stringMap1.json stringMap2.json stringMapResult.json"
  echo "  joins two string maps into resulting json file"
}

if [ $# -lt 3 ]; then
  usage
  exit 0
fi


if [ ! -r $1 ]; then
  echo $1 is not readable file
  usage
  exit 0
fi

if [ ! -r $2 ]; then
  echo $2 is not readable file
  usage
  exit 0
fi

lines=`wc -l $1 | awk '{print $1}' `
start=`expr $lines - 1`
sed "$start,$ d" $1 > $3
echo "    ]," >> $3
sed '1d' $2 >> $3

profile_end ""
