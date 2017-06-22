#!/usr/bin/env bash
#
# cozmo-one/project/build-scripts/generate-assets.sh
#
# Helper script to create unity assets such as Acknowledgements.txt.
# Run (once) by top-level configure.py during generate step.
#

set -e
set -u

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

#
# acknowledgements
#

ACKNOWLEDGEMENTS_BUILD_FILE="${TOPLEVEL}/unity/Cozmo/Assets/Resources/Acknowledgements.txt"
SEPARATOR="\n--------------------------\n"
rm -f "${ACKNOWLEDGEMENTS_BUILD_FILE}"
# can add  ${TOPLEVEL}/licenses/${PLATFORM}/*.license; if platform specific
for license in ${TOPLEVEL}/licenses/*.license
do
    if [ -e "$license" ]; then
        cat $license >> $ACKNOWLEDGEMENTS_BUILD_FILE
        printf $SEPARATOR >> $ACKNOWLEDGEMENTS_BUILD_FILE
    fi
done
