#!/bin/bash

USERNAME=$1
if [ "$USERNAME" == "" ]; then
    echo "Usage: check-username.sh <username>"
    exit 1
fi

if [ "$ACCOUNTS_API_HOST" == "" ]; then
    ACCOUNTS_API_HOST="http://localhost:8000"
fi

if [ "$APP_KEY" == "" ]; then
    APP_KEY=user-key
fi

curl -H "Anki-App-Key: $APP_KEY"  $ACCOUNTS_API_HOST/1/usernames/$USERNAME
