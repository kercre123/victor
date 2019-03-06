#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

docker build -t victor .
docker build -t victor-${USER} -f Dockerfile.dev --build-arg USER=${USER} --build-arg UID=$(id -u $USER) .
docker run --dns-search ankicore.com --dns 192.168.2.30\
       -u ${USER} -it \
       -v ~/:/home/${USER}/:delegated \
       --privileged victor-${USER}
