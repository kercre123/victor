#!/bin/bash

set -e

ROBOT_HOST='root@192.168.43.45'
REMOTE_FOLDER='/data/data/com.anki.victor/cache/crashDumps'

echo "Copying dump files from robot "$ROBOT_HOST
rm -rf crashes
mkdir -p crashes
scp -rp $ROBOT_HOST:$REMOTE_FOLDER/*.dmp crashes/

echo "DONE"
