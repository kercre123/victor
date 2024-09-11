#!/bin/bash

set -e

if [[ ! -f ./CPPLINT.cfg ]]; then
    if [[ -f ../CPPLINT.cfg ]]; then
        cd ..
    else
        echo "This script must be run in the victor repo. ./wire/build.sh"
        exit 1
    fi
fi

if [[ ! -f robot_ip.txt ]]; then
    echo "You must echo your robot's IP to robot_ip.txt in the root of this repo before running this script."
    exit 1
fi

if [[ ! -f robot_sshkey ]]; then
    echo "You must copy your robot's SSH key the root of this repo with the name robot_sshkey before running this script."
    exit 1
fi

chmod 600 robot_sshkey

./docker/deploy-v.sh