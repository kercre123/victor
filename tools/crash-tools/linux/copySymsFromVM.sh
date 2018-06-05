#!/bin/bash

set -e

REMOTE_HOST='pterry@10.10.7.148'
REMOTE_FOLDER='/home/pterry/test'

echo "Copying symbols from VM"
rm -rf symbols
scp -rp $REMOTE_HOST:$REMOTE_FOLDER/symbols/ .

echo "Creating compressed tar file"
rm -f symbols.tar.gz
tar -czf symbols.tar.gz symbols/

echo "DONE"
