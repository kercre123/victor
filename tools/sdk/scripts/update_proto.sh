#!/usr/bin/env bash

set -e

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=`basename ${0}`
TOPLEVEL=$(cd "${SCRIPT_PATH}/../../.." && pwd)
GATEWAY_DIRECTORY="${TOPLEVEL}/cloud/go/src/github.com/grpc-ecosystem/grpc-gateway"
PYTHON_SDK_ROOT="${TOPLEVEL}/tools/sdk/vector-sdk"
PYTHON_GEN_OUT="${PYTHON_SDK_ROOT}/anki_vector/messaging"

# defaults
INCREMENTAL=0

function usage() {
    echo "$SCRIPT_NAME [OPTIONS]"
    echo "  -h  print this message"
    echo "  -v  verbose output"
    echo "  -I  Incremental build (no pip/grpc gateway install)"
}

while getopts "hvI" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        v)
            set -x
            ;;
        I)
            INCREMENTAL=1
            ;;
    esac
done

if [ ${INCREMENTAL} -eq 0 ]; then
    echo "> Downloading required python packages"
    python3 -m pip install grpcio grpcio-tools aiogrpc googleapis-common-protos

    if [ ! -d "${GATEWAY_DIRECTORY}" ]; then
        echo "> Downloading required gRPC Gateway dependency to ${GATEWAY_DIRECTORY}"
        rm -rf "${GATEWAY_DIRECTORY}"
        git clone https://github.com/grpc-ecosystem/grpc-gateway.git ${GATEWAY_DIRECTORY}
    else
        echo "> gRPC Gateway already downloaded"
    fi
fi

echo "> Updating proto files"
mkdir -p ${PYTHON_GEN_OUT}
cp ${TOPLEVEL}/cloud/gateway/*.proto ${PYTHON_GEN_OUT}
sed -i -e 's/import "\([^\/]*\).proto"/import "anki_vector\/messaging\/\1.proto"/g' ${PYTHON_GEN_OUT}/*.proto
rm ${PYTHON_GEN_OUT}/*.proto-e

echo "> Generating pb2 files"
python3 -m grpc_tools.protoc \
    -I ${PYTHON_SDK_ROOT} \
    -I ${TOPLEVEL}/cloud/go/src/github.com/grpc-ecosystem/grpc-gateway/third_party/googleapis \
    --python_out=${PYTHON_SDK_ROOT} \
    --grpc_python_out=${PYTHON_SDK_ROOT} \
    ${PYTHON_GEN_OUT}/*.proto
