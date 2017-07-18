#!/bin/bash
# Copyright (c) 2014 Anki, Inc.
# All rights reserved.

set -e
set -u

: ${HOCKEYAPP_API_TOKEN:=""}
: ${HOCKEYAPP_APPID:=""}
: ${PLATFORM:=""}

function usage() {
    echo "$0 [-h] [-i hockeyapp_appid] [-t hockeyapp_api_token] [-p platform]"
    echo "-h                     : help.  Print out this info"
    echo "-i hockeyapp_appid     : The App ID to upload to"
    echo "-t hockeyapp_api_token : The API token to upload with"
    echo "-p platform            : android or ios"
}

while getopts ":hi:t:p:" opt; do
    case $opt in
        h)
            usage
            exit 0
            ;;
        i)
            HOCKEYAPP_APPID=$OPTARG
            ;;
        t)
            HOCKEYAPP_API_TOKEN=$OPTARG
            ;;
        p)
            PLATFORM=`echo $OPTARG | awk '{print $tolower($1);}'`
            ;;
        :)
            echo "Option -$OPTARG requires an argument." >&2
            usage
            exit 1
            ;;
    esac
done

if [ -z "$HOCKEYAPP_APPID" ]; then
    echo "You must supply a HockeyApp APP ID"
    usage
    exit 1
fi

if [ -z "$HOCKEYAPP_API_TOKEN" ]; then
    echo "You must supply a HockeyApp API token"
    usage
    exit 1
fi

if [ -z "$PLATFORM" ]; then
    echo "You must choose a platform : android, ios"
    usage
    exit 1
fi

if [ "$PLATFORM" != "android" -a "$PLATFORM" != "ios" ]; then
    echo "The only valid platforms are android and ios"
    usage
    exit 1
fi

APK_PATH=`ls stage/artifacts/*.apk 2>/dev/null` || true
APK_LIBS_DEBUG_PATH=`ls stage/artifacts/*.apk-libs-debug.zip 2>/dev/null` || true
IPA_PATH=`ls stage/artifacts/*.ipa 2>/dev/null` || true
DSYM_PATH=`ls stage/artifacts/*dSYM*.zip 2>/dev/null` || true

APP_BUNDLE_PATH=

if [ "$PLATFORM" = "android" ]; then
    if [ -z "$APK_PATH" ]; then
	echo "Error: No APK file found in stage/artifacts/"
	exit 2
    fi
    APP_BUNDLE_PATH=$APK_PATH
fi

if [ "$PLATFORM" = "ios" ]; then
    if [ -z "$IPA_PATH" ]; then
	echo "Error: No IPA file found in stage/artifacts/"
	exit 2
    fi
    APP_BUNDLE_PATH=$IPA_PATH
fi

if [ "$PLATFORM" = "android" -a -n "$APK_LIBS_DEBUG_PATH" ]; then
    echo "Checking for dump_syms...."
    ls stage/bin/dump_syms
    echo "Creating stage/symbols.zip from $APK_LIBS_DEBUG_PATH"
    rm -rf stage/symbols stage/symbols.zip stage/*.so
    unzip -j -o "$APK_LIBS_DEBUG_PATH" -d stage
    pushd stage
    for i in `ls *.so | grep -v .sym.so`; do
        echo Processing ${i}......
        ./bin/dump_syms -v $i . > $i.sym || ./bin/dump_syms -v $i > $i.sym
        VER=`head -1 $i.sym | awk '{print $4;}'`
        mkdir -p symbols/$i/$VER
        mv $i.sym symbols/$i/$VER/
    done
    zip -r symbols.zip symbols
    rm -rf symbols *.so
    popd # stage
    DSYM_PATH="stage/symbols.zip"
fi

CURL_ARGS=" \
-v \
-F ipa=@$APP_BUNDLE_PATH \
-F notify=1 \
-F status=2"


if [ -n "$DSYM_PATH" ]; then
    CURL_ARGS="$CURL_ARGS -F dsym=@$DSYM_PATH"
fi

COMMIT_SHA_PATH=`ls stage/artifacts/commit_sha.txt` || true
if [ -n "$COMMIT_SHA_PATH" ]; then
    CURL_ARGS="$CURL_ARGS -F commit_sha=<$COMMIT_SHA_PATH"
fi

NOTES_PATH=`ls stage/artifacts/notes.txt` || true
if [ -n "$NOTES_PATH" ]; then
    CURL_ARGS="$CURL_ARGS -F notes=<$NOTES_PATH -F notes_type=0"
fi

REP_URL_PATH=`ls stage/artifacts/repository_url.txt` || true
if [ -n "$REP_URL_PATH" ]; then
    CURL_ARGS="$CURL_ARGS -F repository_url=<$REP_URL_PATH"
fi

curl \
$CURL_ARGS \
-H "X-HockeyAppToken: ${HOCKEYAPP_API_TOKEN}" \
"https://rink.hockeyapp.net/api/2/apps/${HOCKEYAPP_APPID}/app_versions/upload"
