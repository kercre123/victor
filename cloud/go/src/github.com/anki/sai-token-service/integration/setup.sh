#!/usr/bin/env sh

aws --profile=integration --endpoint-url http://$AWS_IP:5001 --no-verify-ssl \
    s3 mb s3://anki-vector
