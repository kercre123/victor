import os, sys

try:
    TOKEN = os.environ['DROPBOX_TOKEN'] # Your dropbox token
except KeyError:
    print("Please set environment variable DROPBOX_TOKEN")
    sys.exit(1)

try:
    TABLE_NAME = os.environ['TABLE_NAME_DYNAMO_DB'] # <Your Table name>
except KeyError:
    print("Please set environment variable TABLE_NAME_DYNAMO_DB")
    sys.exit(1)

try:
    REGION_NAME = os.environ['REGION_NAME_DYNAMO_DB'] # <Region of aws dynamodb>
except KeyError:
    print("Please set environment variable REGION_NAME_DYNAMO_DB")
    sys.exit(1)

SECRET_KEY = '7d441f27d441f27567d441f2b6176a' # Random secret key, you can use this key
