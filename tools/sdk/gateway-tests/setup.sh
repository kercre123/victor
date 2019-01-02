#!/usr/bin/env bash

python3 -m pip install pytest requests requests_toolbelt
python3 -m pip install -r ../vector-python-sdk-private/sdk/requirements.txt
rm -f *.proto *_pb2.py *_pb2_grpc.py
../vector-python-sdk-private/update.py -o . -s -p
