#!/usr/bin/env bash

function usage() {
  echo "$SCRIPT_NAME [OPTIONS]"
  echo "options:"
  echo "  -h                      print this message"
  echo "  -s ANKI_ROBOT_HOST      hostname or ip address of robot"
  echo ""
  echo "environment variables:"
  echo '  $ANKI_ROBOT_HOST        hostname or ip address of robot'
}

while getopts "hs:" opt; do
  case $opt in
    h)
      usage && exit 0
      ;;
    s)
      ANKI_ROBOT_HOST="${OPTARG}"
      shift 2
      ;;
    *)
      usage && exit 1
      ;;
  esac
done
