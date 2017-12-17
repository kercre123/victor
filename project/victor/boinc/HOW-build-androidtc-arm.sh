#!/usr/bin/env bash
set -eux

export NDK_ROOT=${HOME}/android-ndk-r15c

cd boinc
cd android

bash build_androidtc_arm.sh
