#!/usr/bin/env bash

set -e

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_FULL_PATH=$(cd "${SCRIPT_PATH}" && pwd)
TOPLEVEL=$(cd "${SCRIPT_PATH}/../.." && pwd)
SUBTREE_REPO="git@github.com:anki/victor-proto.git"
SUBTREE_PREFIX="${SCRIPT_FULL_PATH/${TOPLEVEL}\//}/gateway"

cd $TOPLEVEL
git subtree push --prefix ${SUBTREE_PREFIX} ${SUBTREE_REPO} master