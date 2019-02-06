#!/usr/bin/env bash

set -e

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

pushd $TOPLEVEL/tools/sdk/gateway-tests/

python3 -m pip install pytest requests requests_toolbelt grpcio-tools
python3 -m pip install -r ../vector-python-sdk-private/sdk/requirements.txt
rm -f *.proto *_pb2.py *_pb2_grpc.py
python3 ../vector-python-sdk-private/update.py -o . -s -p -d ../../protobuf/gateway

popd
