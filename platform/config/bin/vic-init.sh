#!/bin/sh -f
#
# platform/config/bin/vic-init.sh
#
# Perform any cleanup & init tasks not handled by the OS.
#
# This script runs each time the service group (anki-robot.target)
# is started, BEFORE any individual vic-blah services.
#

# Clear fault code, if any
/bin/fault-code-clear
