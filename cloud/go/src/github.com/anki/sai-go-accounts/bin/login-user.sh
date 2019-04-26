#!/bin/bash

USERNAME=$1
PASSWORD=$2
if [ "$USERNAME" == "" -o "$PASSWORD" == "" ]; then
    echo "Usage: login-user.sh <userid> <password>"
    exit 1
fi

if [ "$ACCOUNTS_API_HOST" == "" ]; then
    ACCOUNTS_API_HOST="http://localhost:8000"
fi

if [ "$APP_KEY" == "" ]; then
    APP_KEY=user-key
fi

curl -X POST -H "Anki-App-Key: $APP_KEY"  \
    --data-urlencode "username=$USERNAME" \
    --data-urlencode "password=$PASSWORD" \
    $ACCOUNTS_API_HOST/1/sessions
