#!/usr/bin/env bash
#
# Exit for non-zero error
set -e
#
# Exit if a needed environment variable is not set
set -u

# Which spreadsheet are we using?
# Don't use the real sheet unless you know what you are doing!
#spreadsheetId = '1Jci_hz_4xFh_2LuFv-8LfdAvy6d_eRLCJmkxvucdqz4'
spreadsheetId='YOUR-SPREADSHEET-ID-GOES-HERE'

# Which sheet of this spreadsheet are we using?
sheet='YOUR-SHEET-NAME-GOES-HERE'

# Which branches are we comparing?
srcbranch=master
dstbranch=release/candidate

GIT=`which git`
if [ -z $GIT ]; then
  echo git not found
  exit 1
fi

TOPLEVEL=`$GIT rev-parse --show-toplevel`
SCRIPTDIR=$TOPLEVEL/lib/util/tools/branch-scripts

cd $SCRIPTDIR
$SCRIPTDIR/anki-branch-update-spreadsheet.py $spreadsheetId $sheet $srcbranch $dstbranch
