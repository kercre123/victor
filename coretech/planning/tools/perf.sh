#!/bin/bash

# This is a script which runs through a set of environments in the standalone planner to look at the
# performance

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

COUNT=3

CONF=Release

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi

TOPLEVEL_ENGINE=`$GIT rev-parse --show-toplevel`

# echo "Entering directory \`${TOPLEVEL_ENGINE}/..'"
cd $TOPLEVEL_ENGINE/..

TOPLEVEL=`$GIT rev-parse --show-toplevel`

# echo "Entering directory \`${TOPLEVEL}'"
cd $TOPLEVEL

# pwd

DERIVED_DATA_BASE=./build/mac/derived-data

if [ ! -d "${DERIVED_DATA_BASE}" ]; then
    echo "could not find derived data folder, did you build first?"
    exit 1
fi

DERIVED_DATA=`find $DERIVED_DATA_BASE -name 'CozmoWorkspace_mac*' -maxdepth 1`

if [ `echo ${DERIVED_DATA} | wc -l` -ne "1" ]; then
    echo "could not find Cozmo xcode workspace directory in dervied data"
    exit 1
fi

EXE=${DERIVED_DATA}/Build/Products/${CONF}/ctiPlanningStandalone

if [ ! -f ${EXE} ]; then
    echo "Could not find standalone planenr executable ${EXE} (did you build in ${Conf}?)"
    exit 1
fi

# echo "found: $EXE"

# turn the results into a json list
echo "["
for ((i=0; i<$COUNT; i++)); do
    find $DIR/env/context/ -iname '*.json' -exec $EXE -p $DIR/env/mprim.json --context {} -s \; -exec echo "," \;
done
echo "]"




# echo "Leaving directory \`${TOPLEVEL}'"
# echo "Leaving directory \`${TOPLEVEL_ENGINE}'"
