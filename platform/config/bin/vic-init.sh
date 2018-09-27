#!/bin/sh -f
#
# platform/config/bin/vic-init.sh
#
# Perform any cleanup & init tasks not handled by the OS.
#

# Don't let logd prune messages from these UIDs
/usr/bin/logcat -P '0 2901 2902 2903 2904 2905'
