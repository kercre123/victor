#!/usr/bin/env bash
#
# cozmo-one/project/build-scripts/generate-assets.sh
#
# Helper script to create unity assets such as Acknowledgements.txt.
# Run (once) by top-level configure.py during generate step.
#
# We don't want to change mod time on the target file unless the content changes.
# Implement this by generating content into a temp file, then installing
# temp file as target file if needed.
#

set -e
set -u

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

# Where do we put the output?
TARGET="${TOPLEVEL}/unity/Cozmo/Assets/Resources/Acknowledgements.txt"

# Where do we put the temporary file?
TMP=`mktemp -t cozmo-generate-assets`

# Put this marker after each license
SEPARATOR="\n--------------------------\n"

#
# can add  ${TOPLEVEL}/licenses/${PLATFORM}/*.license; if platform specific
#

echo > $TMP
for license in ${TOPLEVEL}/licenses/*.license
do
    if [ -e "$license" ]; then
        cat $license >> $TMP
        printf $SEPARATOR >> $TMP
    fi
done

#
# install -C: Copy the file.  If the target file already exists and the files 
# are the same, then don't change the modification time of the target.
#
install -m 644 -C $TMP "${TARGET}"

rm -f $TMP
