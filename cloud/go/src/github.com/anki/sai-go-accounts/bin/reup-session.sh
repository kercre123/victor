#!/bin/bash

TOKEN=$1
if [ "$TOKEN" == "" ]; then
    echo "Usage: reup-session.sh <token>"
    exit 1
fi

if [ "$ACCOUNTS_API_HOST" == "" ]; then
    ACCOUNTS_API_HOST="http://localhost:8000"
fi

if [ "$APP_KEY" == "" ]; then
    APP_KEY=user-key
fi

curl -X POST -H "Anki-App-Key: $APP_KEY" \
    -H "Authorization: Anki $TOKEN" \
    --data-urlencode "session_token=$TOKEN" \
    $ACCOUNTS_API_HOST/1/sessions/reup
