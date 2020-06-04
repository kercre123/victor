#!/bin/bash

SESSIONID=$1
TOKEN=$2
if [ "$SESSIONID" == "" -o "$TOKEN" == "" ]; then
    echo "Usage: validate-session.sh <sessionid> <token>"
    exit 1
fi

if [ "$ACCOUNTS_API_HOST" == "" ]; then
    ACCOUNTS_API_HOST="http://localhost:8000"
fi

if [ "$APP_KEY" == "" ]; then
    APP_KEY=user-key
fi

curl -k -X POST -H "Anki-App-Key: $APP_KEY" \
    -H "Authorization: Anki $SESSIONID" \
    --data-urlencode "session_token=$TOKEN" \
    $ACCOUNTS_API_HOST/1/sessions/validate
