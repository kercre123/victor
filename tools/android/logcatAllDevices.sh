#!/bin/bash
# Copyright (c) 2014 Anki, Inc.
# All rights reserved.

pushd $(dirname $0)
../../lib/util/tools/android/logcatAllDevices.sh $@
popd
