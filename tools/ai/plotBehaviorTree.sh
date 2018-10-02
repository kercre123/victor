#!/bin/bash

GIT_PROJ_ROOT=`git rev-parse --show-toplevel`

TEXT=`mktemp`

echo ${TEXT}

echo "Map of possible behavior delegation" >> ${TEXT}
echo "An edge is added for each delegate a parent declares" >> ${TEXT}

SHA=`git rev-parse --short HEAD`

echo "Build sha $SHA" >> ${TEXT}

date >> ${TEXT}

if (( $# < 1 )); then
    # use temp file and open to screen
    python2 ${GIT_PROJ_ROOT}/tools/ai/plotBehaviors.py -r InitNormalOperation -v -f -z --text ${TEXT}
else
    # save to input file
    python2 ${GIT_PROJ_ROOT}/tools/ai/plotBehaviors.py -r InitNormalOperation -f -z --text ${TEXT} -o $*
fi

rm ${TEXT}

