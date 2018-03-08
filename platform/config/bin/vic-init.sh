#!/bin/sh -f
#
# platform/config/bin/vic-init.sh
#
# Perform any cleanup & init tasks not handled by the OS.
#

# VIC-1369: Older OS images wrote journal to root partition
echo "Vacuum journal"
/bin/journalctl --vacuum-size=10M

# VIC-1504: Older OS images do not provide init_debuggerd.service
active=`/bin/systemctl is-active init_debuggerd`
if [ "$active" != "active" ]; then
  echo "Enable tombstones"
  /bin/mkdir -p /data/tombstones
  pid=`/bin/pgrep debuggerd`
  if [ "$pid" == "" ]; then
    echo "Start debuggerd"
    /usr/bin/debuggerd > /dev/null 2>&1 &
  fi
fi

#
# FACTORY_TEST
#
# VIC-1500: Enable vic-logmgr.service for factory test
#
active=`/bin/systemctl is-active vic-logmgr.service`
if [ "$active" == "unknown" ]; then
  echo "Enable vic-logmgr.service"
  /bin/systemctl enable --now /anki/etc/systemd/system/vic-logmgr.service
fi
