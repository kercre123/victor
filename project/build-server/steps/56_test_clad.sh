#!/usr/bin/env bash
set -xe

echo "=== Test CLAD ==="

TOPLEVEL=`git rev-parse --show-toplevel`

TESTS_PATH=${TOPLEVEL}/tools/message-buffers/emitters/tests/

PYTHON=python3 make -C ${TESTS_PATH}
PYTHON=python  make -C ${TESTS_PATH}
