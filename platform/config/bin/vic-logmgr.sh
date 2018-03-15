#!/bin/sh -f
#
# platform/config/bin/vic-logmgr.sh
#
# Victor log manager
#
# This script runs as a background to capture android log output
# from other victor services.  /usr/bin/logwrapper is used to provide
# event filtering and log rotation.  Default settings will save up to
# 64MB of the most recent log data.
#
# The default filter specification will capture Anki events, warnings,
# and errors. It will NOT capture Anki info or Anki debug messages.
# We also capture anything reported at Android ERROR or FATAL level,
# including tombstones reported by debuggerd.
#
# Default configuration values may be overridden by environment.
# When run from vic-logmgr.service, environment values may
# be set in /anki/etc/vic-logmgr.env.
#
# Note that Android log levels are not the same as Anki log levels!
#   Android ERROR = Anki ERROR
#   Android WARNING = Anki WARNING
#   Android INFO = Anki EVENT
#   Android DEBUG = Anki INFO
#   Android VERBOSE = Anki DEBUG
#

: ${VIC_LOGMGR_DIRECTORY:="/data/data/com.anki.victor/cache/vic-logmgr"}
: ${VIC_LOGMGR_FILENAME:="vic-logmgr"}
: ${VIC_LOGMGR_ROTATION_KB:="1024"}
: ${VIC_LOGMGR_ROTATION_COUNT:="64"}
: ${VIC_LOGMGR_FORMAT:="printable"}
: ${VIC_LOGMGR_FILTERSPECS:="vic-robot:V vic-anim:V vic-engine:V vic-webserver:V vic-cloud:V vic-switchboard:V *:E"}

/bin/mkdir -p ${VIC_LOGMGR_DIRECTORY}

exec /usr/bin/logcat \
   -f ${VIC_LOGMGR_DIRECTORY}/${VIC_LOGMGR_FILENAME} \
   -r ${VIC_LOGMGR_ROTATION_KB} \
   -n ${VIC_LOGMGR_ROTATION_COUNT} \
   -v ${VIC_LOGMGR_FORMAT} \
   ${VIC_LOGMGR_FILTERSPECS}
