#!/usr/bin/env bash
#
# project/victor/scripts/start-lldb.sh
#
# Helper script to enable use of lldb-server on victor robot
#
set -u

: ${TOPLEVEL:=$(git rev-parse --show-toplevel)}

source ${TOPLEVEL}/project/victor/scripts/victor_env.sh

source ${TOPLEVEL}/project/victor/scripts/host_robot_ip_override.sh

robot_set_host

# Is LLDB already running?
LLDB_SERVER_PID=$(robot_sh pidof lldb-server)
if [ ! -z "${LLDB_SERVER_PID}" ]; then
  echo "lldb-server PID ${LLDB_SERVER_PID} is already running"
  exit 0
fi

#
# Allow write to root filesystem
# This is needed for deployment of lldb-server and for process launch.
#
robot_sh "mount -o rw,remount /"
if [ $? -ne 0 ]; then
  echo "Unable to remount root partition"
  exit 1
fi

#
# Deploy to robot?  We put the executable into /usr/bin so that it will not be
# affected by deployments into /anki.
#
DEPLOY_FROM="${TOPLEVEL}/3rd/lldb-server/vicos/bin/lldb-server"
DEPLOY_TO="/usr/bin/lldb-server"

robot_sh "test -f ${DEPLOY_TO}"
if [ $? -ne 0 ]; then
  echo "Deploy lldb-server to ${ANKI_ROBOT_HOST}"
  robot_cp ${DEPLOY_FROM} ${DEPLOY_TO}
  if [ $? -ne 0 ]; then
    echo "Failed to deploy lldb-server to ${ANKI_ROBOT_HOST}"
    exit 1
  fi
fi

#
# Open firewall on all ports.  LLDB listens for connections on a specific port,
# but it will allocate an additional port each time you attach to a process.
# A better version would use a limited port range instead of allowing access on all ports.
#
robot_sh "sh /etc/iptables/iptables-flush.sh"
if [ $? -ne 0 ]; then
  echo "Failed to open firewall on ${ANKI_ROBOT_HOST}"
  exit 1
fi

#
# Start the server process
#
# The lldb-server executable is built with "RPATH=$ORIGIN/../lib".
# When installed in /usr/bin, it is able to find shared libraries in /usr/lib,
# but it is not able to find libc++.so.6 installed into /anki/lib.
# We start the server process with LD_LIBRARY_PATH set to search both /usr/lib and /anki/lib.
#

PORT="55001"

echo "Starting lldb-server on ${ANKI_ROBOT_HOST}:${PORT}"

robot_sh "LD_LIBRARY_PATH=/usr/lib:/anki/lib nohup logwrapper -- ${DEPLOY_TO} platform --server --listen *:${PORT} </dev/null >/dev/null 2>&1 &"
if [ $? -ne 0 ]; then
  echo "${DEPLOY_TO} failed to start"
  exit 1
fi

echo "${DEPLOY_TO} now running on ${ANKI_ROBOT_HOST}:${PORT}"
