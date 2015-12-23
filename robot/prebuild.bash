#!/usr/bin/env bash
export PATH=/usr/bin:$PATH
make cpplilib -C clad
bash tools/versionGenerator/versionGenerator.sh include/anki/cozmo/robot/version.h
