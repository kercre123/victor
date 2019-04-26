#!/bin/bash

SESSIONID=$1
USERID=$2
if [ "$SESSIONID" == "" -o "$USERID" == "" ]; then
    echo "Usage: get-user.sh <sessionid> <userid>"
    exit 1
fi


if [ "$ACCOUNTS_API_HOST" == "" ]; then
    ACCOUNTS_API_HOST="http://localhost:8000"
fi

if [ "$APP_KEY" == "" ]; then
    APP_KEY=user-key
fi

curl -H "Anki-App-Key: $APP_KEY"  \
    -H "Authorization: Anki $SESSIONID" \
    $ACCOUNTS_API_HOST/1/users/$USERID
