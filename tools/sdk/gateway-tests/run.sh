#!/usr/bin/env bash

set -e

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=`basename ${0}`
TOPLEVEL=$(cd "${SCRIPT_PATH}/../../.." && pwd)
GATEWAY_DIRECTORY="${TOPLEVEL}/cloud/go/src/github.com/grpc-ecosystem/grpc-gateway"
PYTHON_SDK_ROOT="${TOPLEVEL}/tools/sdk/vector-sdk"
VERBOSE=
ENV_ROOT="/tmp"

function usage() {
    echo "$SCRIPT_NAME [OPTIONS]"
    echo "  -h  print this message"
    echo "  -e  root to use for virtualenv"
    echo "  -v  verbose output"
}

while getopts ":e:hv" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        e)
            ENV_ROOT="${OPTARG}"
            ;;
        v)
            VERBOSE=1
            ;;
    esac
done

if [ ! -z "$VERBOSE" ]
then
    set -x
fi

ENV_DIR="${ENV_ROOT}/test-gateway-env"

if [ -t 1 ]
then
    RED="$(tput setaf 1)"
    GREEN="$(tput setaf 2)"
    YELLOW="$(tput setaf 3)"
    BLUE="$(tput setaf 4)"
    PURPLE="$(tput setaf 5)"
    CYAN="$(tput setaf 6)"
    CLEAR="$(tput sgr0)"
else
    RED=
    GREEN=
    YELLOW=
    BLUE=
    PURPLE=
    CYAN=
    CLEAR=
fi

function title(){
    echo ${CYAN}$@${CLEAR}
}

title "[ Clearing virtualenv: '${CLEAR}${ENV_DIR}${CYAN}' ]"
rm -rf ${ENV_DIR}
virtualenv -p python3 ${ENV_DIR} --no-site-packages
source ${ENV_DIR}/bin/activate
title "[ Installing requirements ]"
pip install -q -r ${SCRIPT_PATH}/requirements.txt
title "[ Installing current anki_vector ]"
pip install -q ${PYTHON_SDK_ROOT}
title "[ Running gateway tests ]"
pytest -v
