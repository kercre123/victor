#!/usr/bin/env bash
set -eux


export NDK_ROOT=${HOME}/android-ndk-r15c

export BOINC=${HOME}/BOINC
export OPENSSL_SRC=${BOINC}/openssl-1.0.0d
export CURL_SRC=${BOINC}/curl-7.27.0

cd boinc
cd android

#bash build_androidtc_arm.sh
bash build_openssl_arm.sh

