#!/bin/bash

PARAM=$1
if [ "$PARAM" == ""  ]; then
    echo "Usage: reset-password-<userid> username=<username> | email=<email>"
    exit 1
fi

if [ "$ACCOUNTS_API_HOST" == "" ]; then
    ACCOUNTS_API_HOST="http://localhost:8000"
fi

if [ "$APP_KEY" == "" ]; then
    APP_KEY=user-key
fi

curl -X POST -H "Anki-App-Key: $APP_KEY"  \
    --data-urlencode "$PARAM" \
    $ACCOUNTS_API_HOST/1/reset_user_password
