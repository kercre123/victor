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
TARGET="${TOPLEVEL}/unity/Cozmo/Assets/Resources/Acknowledgements"

mkdir -p $TARGET

for license in ${TOPLEVEL}/licenses/*.license
do
    if [ -e "$license" ]; then
      filename=`basename $license ".license"`
      target_path=$TARGET/$filename".txt"
      #only copy if newer, cp doesn't support -u on mac.
      rsync -u $license $target_path
    fi
done

