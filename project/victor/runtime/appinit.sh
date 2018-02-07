#!/system/bin/sh
#
# Victor startup script to be run from init.rc.
# Assumes that /system and /data have been mounted.
#

#
# Suppress qcamera log spam
#   https://source.codeaurora.org/quic/la/platform/hardware/qcom/camera/tree/QCamera2/stack/mm-camera-interface/src/mm_camera_interface.c?h=LA.BR.1.2.9_rb1.15#n1504
#
setprop persist.camera.hal.debug.mask 7

#
# Start Anki services
#
robotctl=/data/data/com.anki.cozmoengine/robotctl.sh
animctl=/data/data/com.anki.cozmoengine/animctl.sh
cozmoctl=/data/data/com.anki.cozmoengine/cozmoctl.sh
cloudctl=/data/data/com.anki.cozmoengine/cloudctl.sh
webserverctl=/data/data/com.anki.cozmoengine/webserverctl.sh

if [ -x $robotctl ]; then
  $robotctl restart
fi

if [ -x $animctl ]; then
  $animctl restart
fi

if [ -x $cozmoctl ]; then
  $cozmoctl restart
fi

if [ -x $cloudctl ]; then
  $cloudctl restart
fi

if [ -x $webserverctl ]; then
  $webserverctl restart
fi

