#!/bin/sh
cd /anki

if [ $1 == forever ]; then
  while true; do
    tinyplay square-05.wav
  done
fi

/etc/initscripts/anki-audio-init
./testsound.sh forever &