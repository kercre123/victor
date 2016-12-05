#!/usr/bin/env bash
set -xe

echo "=== Test CLAD ==="

#${SCRIPT_PATH}/

TESTS_PATH=../../../tools/message-buffers/emitters/tests/

PYTHON=python3 make -C ${TESTS_PATH}
PYTHON=python  make -C ${TESTS_PATH}
