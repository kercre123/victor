#!/usr/bin/env bash

GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
source ${GIT_PROJ_ROOT}/project/victor/scripts/android_env.sh

$ADB shell rm -rf /data/data/com.anki.*
$ADB shell rm -rf /anki
