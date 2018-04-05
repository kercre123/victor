#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

if [ ! -d "node_modules" ]; then
  ./install.sh
fi

expect -f vic_show_ip.expect $1
