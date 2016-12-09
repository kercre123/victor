#!/bin/bash

set -e
set -u

if [ ! -d standalone-apk ]; then
    echo "Must be run from top level directory"
    exit 1
fi

./standalone-apk/rebuild-cozmo-app.sh

./standalone-apk/stage-assets.sh

buck build :cozmoengine_standalone_app
