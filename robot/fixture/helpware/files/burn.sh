#!/bin/sh
rm -rf /data/burn/
killall -9 burn
mkdir -p /data/burn/
/anki/burn 0 &
/anki/burn 1 &
/anki/burn 2 &
/anki/burn 3 &
/anki/burn 0 disk &
/anki/burn 1 disk &
/anki/burn 2 disk &
/anki/burn 3 disk &
