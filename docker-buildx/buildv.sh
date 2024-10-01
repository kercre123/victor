#!/bin/bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

# Ensure buildx builder exists
if ! docker buildx inspect vicbuilder > /dev/null 2>&1; then
    docker buildx create --name vicbuilder --use
else
    docker buildx use vicbuilder
fi

# Build the combined image using buildx
docker buildx build --load -t victor-${USER} \
  --build-arg USER=${USER} \
  --build-arg UID=$(id -u $USER) \
  --build-arg VIC_DIR=${DIR}/../ .

# Run the container
docker run \
  -u ${USER} \
  -v ~/:/home/${USER}/:delegated \
  --privileged victor-${USER}
