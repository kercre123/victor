#!/bin/bash

SESSIONID=$1
USERID=$2
FIELDNAME=$3
FIELDVALUE=$4
if [ "$SESSIONID" == "" -o "$USERID" == "" -o "$FIELDNAME" == "" -o "$FIELDVALUE" == "" ]; then
    echo "Usage: edit-user-field.sh <sessionid> <userid> <field_name> <field_value>"
    exit 1
fi


if [ "$ACCOUNTS_API_HOST" == "" ]; then
    ACCOUNTS_API_HOST="http://localhost:8000"
fi

if [ "$APP_KEY" == "" ]; then
    APP_KEY=user-key
fi

curl -X PATCH -H "Anki-App-Key: $APP_KEY" -H "Content-Type: application/json;charset=utf-8" \
    -H "Authorization: Anki $SESSIONID" \
    --data @- $ACCOUNTS_API_HOST/1/users/$USERID <<EOF
{ "$FIELDNAME": "$FIELDVALUE" }
EOF
