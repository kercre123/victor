#!/usr/bin/env bash
#
# lib/util/tools/branch-scripts/victor-branch-update-spreadsheet.sh
#
# Helper script to update branch management spreadsheet
#
# Exit for non-zero error
set -e
#
# Exit if a needed environment variable is not set
set -u

# Which spreadsheet are we using?
# Don't use the real sheet unless you know what you are doing!
#spreadsheetId = '1IMYSIia8QQhIJ7V5zeXrVawHGYD2uo1i_2Qeej0GKTA'
spreadsheetId='YOUR-SPREADSHEET-ID-GOES-HERE'

# Which sheet of this spreadsheet are we using?
sheet='YOUR-SHEET-NAME-GOES-HERE'

# Which branches are we comparing?
srcbranch=master
dstbranch=release/candidate

GIT=`which git`
if [ -z ${GIT} ]; then
  echo git not found
  exit 1
fi

TOPLEVEL=`${GIT} rev-parse --show-toplevel`
SCRIPTDIR=${TOPLEVEL}/lib/util/tools/branch-scripts

cd ${SCRIPTDIR}
${SCRIPTDIR}/anki-branch-update-spreadsheet.py ${spreadsheetId} ${sheet} ${srcbranch} ${dstbranch}
