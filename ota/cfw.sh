#!/bin/bash

echo "Detecting software state..."

DVT3=0
ANKIDEV=0
OSKR=0

if [[ ! -f /build.prop ]]; then
    echo "This isn't VicOS!"
    exit 1
fi

if [[ -f /system/bin/sh ]]; then
    echo "This script cannot work on Android (VicOS LA) at the moment."
    exit 1
fi

if [[ "$(cat /proc/cmdline)" =~ "anki.dev" ]]; then
    ANKIDEV=1
fi

if [[ -f /bin/emr-cat ]]; then
    if [[ $(emr-cat e) =~ "00e0"]]; then
        DVT3=1
    elif [[ $(emr-cat e) =~ "00e2"]]; then
        OSKR=0
    else
        OSKR=1
    fi
else
    # assume DVT3 or under
    DVT3=1
fi

if ANKIDEV; then
    
fi