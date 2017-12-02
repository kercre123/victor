#!/usr/bin/env bash

# First argument is function to run on all robots
# Second argument is repo root directory

funcs=(download_log update)

if [ $# -ne 2 ]; then
    echo "No function specified, supported functions are"
    echo ${funcs[*]}
    exit 1
fi

if [[ " ${funcs[*]} " == *" $1 "* ]]; then
    if [ -n "$1" ]; then
        echo "running run_on_all_robot_src/$1.sh"
        expect -f "run_on_all_robot_src/$1.sh" $2
    else
        echo "No config found for robot $1"
    fi
else
  echo "Unsupported function, supported functions are"
  echo ${funcs[*]}
  exit 1
fi