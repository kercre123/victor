#!/system/bin/sh
#
# Victor startup script to be run from init.rc.
# Assumes that /system and /data have been mounted.
#
robotctl=/data/data/com.anki.cozmoengine/robotctl.sh
animctl=/data/data/com.anki.cozmoengine/animctl.sh
cozmoctl=/data/data/com.anki.cozmoengine/cozmoctl.sh

if [ -x $robotctl ]; then
  $robotctl restart
fi

if [ -x $animctl ]; then
  $animctl restart
fi

if [ -x $cozmoctl ]; then
  $cozmoctl restart
fi

