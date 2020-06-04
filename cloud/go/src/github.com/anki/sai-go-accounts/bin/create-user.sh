#!/bin/bash

USERNAME=$1
PASSWORD=$2
EMAIL=$3
if [ "$USERNAME" == "" -o "$PASSWORD" == "" -o "$EMAIL" == "" ]; then
    echo "Usage: create-user.sh <username> <password> <email>"
    exit 1
fi


if [ "$ACCOUNTS_API_HOST" == "" ]; then
    ACCOUNTS_API_HOST="http://localhost:8000"
fi

if [ "$APP_KEY" == "" ]; then
    APP_KEY=user-key
fi

if [ "$EMAIL_LANG" != "" ]; then
    EMAIL_LANG_PART=",\"email_lang\":\"$EMAIL_LANG\" "
fi

if [ "$DOBTYPE" = "adult" ]; then
    DOB_PART=",\"dob\":\"1970-01-01\" "
fi

POSTDATA=`cat <<EOF
{ "username": "$USERNAME", "password": "$PASSWORD", "email": "$EMAIL" $DOB_PART $EMAIL_LANG_PART }
EOF`

echo $POSTDATA

curl -X POST -H "Anki-App-Key: $APP_KEY" \
    -H "Content-Type: application/json;charset=utf-8" \
    --data @- $ACCOUNTS_API_HOST/1/users <<EOF
$POSTDATA
EOF
