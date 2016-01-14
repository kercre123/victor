#!/bin/bash
# Copyright (c) 2014 Anki, Inc.
# All rights reserved.

set -e
set -u

if [ $# -ne 4 ]
  then
    echo "Usage: $0 HOCKEYAPP_API_TOKEN HOCKEYAPP_APPID IPA_PATH SYMBOLS_PATH"
    exit 1
fi

COMMIT_SHA_FROM_GIT=`git rev-parse --short HEAD`
REPOSITORY_URL=`git config remote.origin.url`

if [ ! -f "notes.txt" ]; then
    echo "+Last Commit+" > notes.txt
    git log -1 --format="*%h* %s" --abbrev-commit > notes.txt
fi

# Set POST symbol key based on symbol file type.
# HockeyApp required 'dsym' for iOS, or 'libs' for Android
SYMBOL_KEY="libs"
if [[ $4 == *.dSYM.zip ]]; then SYMBOL_KEY="dsym"; fi

curl \
    -v \
    -F "ipa=@$3" \
    -F "$SYMBOL_KEY=@$4" \
    -F "notify=1" \
    -F "status=2" \
    -F "commit_sha=${COMMIT_SHA_FROM_GIT}" \
    -F "repository_url=${REPOSITORY_URL}" \
    -F "notes=<notes.txt" \
    -F "notes_type=0" \
    -H "X-HockeyAppToken: $1" \
    https://rink.hockeyapp.net/api/2/apps/$2/app_versions/upload

rm -f notes.txt
