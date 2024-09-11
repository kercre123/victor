#!/bin/bash

if [[ ! -f ./CPPLINT.cfg ]]; then
    if [[ -f ../CPPLINT.cfg ]]; then
        cd ..
    else
        echo "This script must be run in the victor repo. ./wire/build.sh"
        exit 1
    fi
fi

rm -rf _build generated

echo "cleaned"