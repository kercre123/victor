#!/bin/sh -f
#
# platform/config/bin/vic-init.sh
#
# Perform any cleanup & init tasks not handled by the OS.
#
# This script runs each time the service group (anki-robot.target)
# is started, BEFORE any individual vic-blah services.
#

#
# Forward log events from rampost, if any
#
RAMPOST_LOG=/dev/rampost.log

if [ -f ${RAMPOST_LOG} ]; then
  /anki/bin/vic-log-forward rampost ${RAMPOST_LOG}
  /bin/rm -f ${RAMPOST_LOG}
fi

#
# Clear fault code, if any
#
/bin/fault-code-clear
