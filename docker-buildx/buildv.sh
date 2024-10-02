#!/bin/bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

CACHE_DIR="${HOME}/.build-docker"

if [ ! -d "$CACHE_DIR" ]; then
    mkdir -p "$CACHE_DIR"
fi

if ! docker buildx inspect vicbuilder > /dev/null 2>&1; then
    docker buildx create --name vicbuilder --use
else
    docker buildx use vicbuilder
fi

docker buildx build --load -t victor-"${USER}" \
  --build-arg USER="${USER}" \
  --build-arg UID="$(id -u "${USER}")" \
  --build-arg VIC_DIR="${DIR}/../" \
  --cache-from type=local,src="${CACHE_DIR}" \
  --cache-to type=local,dest="${CACHE_DIR}" \
  .

docker run \
  -u "${USER}" \
  -v ~/:/home/"${USER}"/:delegated \
  --privileged victor-"${USER}"
