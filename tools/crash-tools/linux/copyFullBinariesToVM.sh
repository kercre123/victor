#!/bin/bash

set -e

REMOTE_HOST='pterry@10.10.7.148'
DEST_FOLDER='/home/pterry/test'
SRC_FOLDER='../github/Victor/_build/vicos/Release'

echo "Copying *.full files from here to my VM"
ssh $REMOTE_HOST "rm -rf $DEST_FOLDER/lib"
ssh $REMOTE_HOST "rm -rf $DEST_FOLDER/bin"
ssh $REMOTE_HOST "mkdir $DEST_FOLDER/lib"
ssh $REMOTE_HOST "mkdir $DEST_FOLDER/bin"
scp $SRC_FOLDER/lib/*.full $REMOTE_HOST:$DEST_FOLDER/lib/
scp $SRC_FOLDER/bin/*.full $REMOTE_HOST:$DEST_FOLDER/bin/
echo "DONE"
