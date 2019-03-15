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

# Deploy to robot?
DEPLOY_FROM="${TOPLEVEL}/EXTERNALS/lldb-server/vicos/bin/lldb-server"
DEPLOY_TO="/anki/bin/lldb-server"

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
# A better version would use a limited port range instead of allowing the whole world in.
#
robot_sh "iptables --policy INPUT ACCEPT"
if [ $? -ne 0 ]; then
  echo "Failed to open firewall on ${ANKI_ROBOT_HOST}"
  exit 1
fi

#
# Start the server process
#
PORT="55001"

echo "Starting lldb-server on ${ANKI_ROBOT_HOST}:${PORT}"

robot_sh "nohup ${DEPLOY_TO} platform --server --listen *:${PORT} </dev/null >/dev/null 2>&1 &"
if [ $? -ne 0 ]; then
  echo "${DEPLOY_TO} failed to start"
  exit 1
fi

echo "${DEPLOY_TO} now running on ${ANKI_ROBOT_HOST}:${PORT}"
