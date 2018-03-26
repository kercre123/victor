#!/bin/bash

tar -pczf deployables.tar.gz -C _build/android/Release bin lib etc data \
                             -C ../../../tools/ rsync \
                             -C ../project/victor/scripts victor_env.sh deploy_build_artifacts.sh
