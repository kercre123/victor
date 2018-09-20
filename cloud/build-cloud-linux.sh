#!/bin/bash

# This script can be used to build cloud targets for Linux / x64 with minimal dependencies. This
# is used for e.g. running (integration tests) or the cloud process on a development machine

SCRIPT_PATH="$( cd "$(dirname "$0")"/.. ; pwd -P )"
SCRIPT_NAME=`basename ${0}`
OUTPUT_DIR=${SCRIPT_PATH}/_build/cloud

# initialize some defaults
COMMAND="build"
TARGET_DIR="process"

CHIPPER_SECRET="zzz"

function usage() {
    echo "$SCRIPT_NAME [OPTIONS]"
    echo "  -h                      print this message"
    echo "  -c [COMMAND]            command (i.e. 'test' or 'build')"
    echo "  -d [TARGET_DIR]         source directory to build, e.g. 'process' or 'logcollectorcli'"
    echo "  -n [TARGET_NAME]        name for the target (executable), e.g.: 'vic-cloud' or 'logcollectorcli'"
}

while getopts ":c:n:d:h" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        c)
            COMMAND="${OPTARG}"
            ;;
        n)
            TARGET_NAME="${OPTARG}"
            ;;
        d)
            TARGET_DIR="${OPTARG}"
            ;;
        :)
            echo "Option -${OPTARG} required an argument." >&2
            usage
            exit 1
            ;;
    esac
done

# Set target name equal to dir name if not set via command line
if [ -z ${TARGET_NAME+x} ]; then
    TARGET_NAME=$(basename $TARGET_DIR)
fi

if [ ! -d ${SCRIPT_PATH}/generated/cladgo ]; then
    echo "ERROR: required CLAD source files not generated (use regular build environment scripts)"
    exit 1
fi

if [ ! -f /usr/include/opus/opus.h ]; then
    echo "ERROR: OPUS development headers / libraries not installed (e.g. apt-get install libopus-dev)"
    exit 1
fi

if [ ! -d "$TARGET_DIR" ]; then
    echo "ERROR: source directory ${TARGET_DIR} does not exist)"
    exit 1
fi


# Export environment variables for go build
export CGO_ENABLED=1
export GOCACHE=off

export GOPATH=${SCRIPT_PATH}/cloud/go:${SCRIPT_PATH}/generated/cladgo:${SCRIPT_PATH}/generated/go:${SCRIPT_PATH}/tools/message-buffers/support/go
export CGO_FLAGS=-g
export CGO_CPPFLAGS=-I\ ${SCRIPT_PATH}/lib/util/source/anki/util/..\ -I\ /usr/include/opus\ -I\ ${SCRIPT_PATH}
export CGO_CXXFLAGS=-std=c++14
export CGO_LDFLAGS=-lopus

echo "Starting ${COMMAND} command for ${TARGET_NAME}"

TARGET_PATH=${OUTPUT_DIR}/${TARGET_NAME}

if [ $COMMAND = "test" ]; then
    TARGET_PATH=${TARGET_PATH}.test
    go ${COMMAND} \
        -c -o ${TARGET_PATH} \
        -pkgdir ${SCRIPT_PATH}/_build/cloud/pkgdir \
        -ldflags -X\ \'anki/voice.ChipperSecret=${CHIPPER_SECRET}\'\ \
        ./${TARGET_DIR}
else
    go ${COMMAND} \
        -o ${TARGET_PATH} \
        -pkgdir ${SCRIPT_PATH}/_build/cloud/pkgdir \
        -ldflags -X\ \'anki/voice.ChipperSecret=${CHIPPER_SECRET}\'\ \
        ./${TARGET_DIR}
fi

if [ $? -eq 0 ]; then
    echo "Binary is written to ${TARGET_PATH}"
else
    echo "ERROR: ${COMMAND} step failed!"
fi
