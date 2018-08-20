#!/bin/bash

set -e
set -u

pushd `dirname $0`/../../.git/hooks > /dev/null
[ ! -e pre-commit ] && ln -s ../../tools/hooks/pre-commit
popd > /dev/null
