#!/system/bin/sh

function kill_all()
{
  kill ${ROBOT_PID}
  kill ${ANIM_PID}
  kill ${ENGINE_PID}
  kill ${CLOUD_PID}
  kill ${WEBSERVER_PID}

  sleep 1

  # Use the display program to write the process name to the face in font 2, layer 2 and in white
  echo "2 2 w $1" | display > /dev/null 2>&1

  exit 0
}

function stop()
{
  # Stop all existing watchers
  WATCHER_PIDS=($(pgrep -f watcher.sh | grep -v $$))

  for i in "${WATCHER_PIDS[@]}"; do
    kill $i
  done

  # Clear the display
  display > /dev/null 2>&1

  exit 0
}

function start()
{
  ROBOT_PID="$(/data/data/com.anki.cozmoengine/robotctl.sh get_pid)"
  ROBOT_NAME="$(/data/data/com.anki.cozmoengine/robotctl.sh get_name)"
  ANIM_PID="$(/data/data/com.anki.cozmoengine/animctl.sh get_pid)"
  ANIM_NAME="$(/data/data/com.anki.cozmoengine/animctl.sh get_name)"
  ENGINE_PID="$(/data/data/com.anki.cozmoengine/cozmoctl.sh get_pid)"
  ENGINE_NAME="$(/data/data/com.anki.cozmoengine/cozmoctl.sh get_name)"
  CLOUD_PID="$(/data/data/com.anki.cozmoengine/cloudctl.sh get_pid)"
  CLOUD_NAME="$(/data/data/com.anki.cozmoengine/cloudctl.sh get_name)"
  WEBSERVER_PID="$(/data/data/com.anki.cozmoengine/webserverctl.sh get_pid)"
  WEBSERVER_NAME="$(/data/data/com.anki.cozmoengine/webserverctl.sh get_name)"

  while true; do

    kill -0 ${ROBOT_PID} > /dev/null 2>&1
    ROBOT_RUNNING=$?

    kill -0 ${ENGINE_PID} > /dev/null 2>&1
    ENGINE_RUNNING=$?

    kill -0 ${ANIM_PID} > /dev/null 2>&1
    ANIM_RUNNING=$?

    kill -0 ${CLOUD_PID} > /dev/null 2>&1
    CLOUD_RUNNING=$?

    kill -0 ${WEBSERVER_PID} > /dev/null 2>&1
    WEBSERVER_RUNNING=$?

    if [ "${ROBOT_RUNNING}" -ne "0" ] && [ "${ENGINE_RUNNING}" -ne "0" ] && [ "${ANIM_RUNNING}" -ne "0" ] && [ "${CLOUD_RUNNING}" -ne "0" ] && [ "${WEBSERVER_RUNNING}" -ne "0" ]; then
      exit 0
    fi

    if [ "${ROBOT_RUNNING}" -ne "0" ]; then
      kill_all ${ROBOT_NAME}
    fi

    if [ "${ANIM_RUNNING}" -ne "0" ]; then
      kill_all ${ANIM_NAME}
    fi

    if [ "${ENGINE_RUNNING}" -ne "0" ]; then
      kill_all ${ENGINE_NAME}
    fi

    if [ "${CLOUD_RUNNING}" -ne "0" ]; then
      kill_all ${CLOUD_NAME}
    fi

    if [ "${WEBSERVER_RUNNING}" -ne "0" ]; then
      kill_all ${WEBSERVER_NAME}
    fi

    sleep 5
  done
}

ERROR=0
case $* in
    start)
        start
        ERROR=$?
        ;;
    stop)
        stop
        ERROR=$?
        ;;
    *)
        echo "unknown command: $*"
        ERROR=1
        ;;
esac

exit $ERROR