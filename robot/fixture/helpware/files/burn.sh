#!/bin/sh
rm -rf /data/burn/
killall -9 burn
mkdir -p /data/burn/
/anki/menuman/burn 0 &
/anki/menuman/burn 1 &
/anki/menuman/burn 2 &
/anki/menuman/burn 3 &
/anki/menuman/burn 0 disk &
/anki/menuman/burn 1 disk &
/anki/menuman/burn 2 disk &
/anki/menuman/burn 3 disk &
