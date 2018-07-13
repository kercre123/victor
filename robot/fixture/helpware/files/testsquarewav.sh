#!/bin/sh
killall -9 pcm && exit
/etc/initscripts/anki-audio-init
export LD_LIBRARY_PATH=/anki/menuman
/anki/menuman/pcm -f 440 --square &