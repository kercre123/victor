#!/bin/sh

if [ "$1" == "" ]; then
  /anki/menuman/testsquarewav.sh async &
  exit 0
fi

killall -9 pcm && exit
/etc/initscripts/anki-audio-init
export LD_LIBRARY_PATH=/anki/menuman
/anki/menuman/pcm -f 440 --square &