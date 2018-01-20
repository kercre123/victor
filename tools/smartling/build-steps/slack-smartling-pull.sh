#!/bin/bash
#
# Download strings files from smartling using buildbot and open Pull Request
#

set -e
set -u

send_slack_message() {
_payload="{
              \"channel\": \"$SLACK_CHANNEL\",
              \"username\": \"buildbot\",
              \"attachments\": [
                {
                  \"text\": \"$1\",
                  \"fallback\": \"$1\",
                  \"color\": \"$2\"
                }
              ]
            }"

    curl -X POST --data-urlencode "payload=$_payload" $SLACK_TOKEN_URL

    exit $3
}

send_slack_message_no_exit() {
_payload="{
              \"channel\": \"$SLACK_CHANNEL\",
              \"username\": \"buildbot\",
              \"attachments\": [
                {
                  \"text\": \"$1\",
                  \"fallback\": \"$1\",
                  \"color\": \"$2\"
                }
              ]
            }"

    curl -X POST --data-urlencode "payload=$_payload" $SLACK_TOKEN_URL
}

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi

CURL=`which curl`
if [ -z $CURL ];then
  echo curl not found
  exit 1
fi

PYTHON=`which python2`
if [ -z $PYTHON ];then
  echo python2 not found
  exit 1
fi

PYTHON3=`which python3`
if [ -z $PYTHON3 ];then
  echo python3 not found
  exit 1
fi

_TOPLEVEL=`$GIT rev-parse --show-toplevel`
_SMARTLING_PULL_SCRIPT=${_TOPLEVEL}/tools/smartling/smartling-pull.sh
_GENERATE_SDF_SCRIPT=${_TOPLEVEL}/tools/smartling/generateSDFfromTranslations.py
_STRIP_SPECIAL_CHARACTERS=${_TOPLEVEL}/tools/smartling/smartling-strip-special-characters.py
_CHECK_JSON_KEYS=${_TOPLEVEL}/tools/smartling/smartling-check-json-keys.py
_UNITY_PROJECT_DIR=${_TOPLEVEL}/unity/Cozmo/
_TRANSLATED_ASSETS_DIR=Assets/StreamingAssets/LocalizedStrings/ja-JP
_ASSET_BUNDLES_DIR=${_TOPLEVEL}/unity/Cozmo/Assets/AssetBundles
_TRANSLATED_OUTPUT_FILE=Assets/DevTools/Editor/LanguageHelpers/japaneseTranslations.txt
_LOCALIZED_STRINGS_DIR=${_TOPLEVEL}/unity/Cozmo/Assets/StreamingAssets/LocalizedStrings
_GIT_COZMO_URI=https://$ANKI_SMARTLING_ACCESS_TOKEN:x-oauth-basic@github.com/anki/cozmo-one.git
_GIT_COZMO_PR_URI=https://$ANKI_SMARTLING_ACCESS_TOKEN:x-oauth-basic@api.github.com/repos/anki/cozmo-one/pulls

exit_status=0
$_SMARTLING_PULL_SCRIPT -t pending $_LOCALIZED_STRINGS_DIR || exit_status=$?
if [ $exit_status -ne 0 ]; then
    send_slack_message "There was a problem downloading *.json from Smartling. Check build log!" "danger" $exit_status
fi

$PYTHON3 $_STRIP_SPECIAL_CHARACTERS --localized-strings-dir $_LOCALIZED_STRINGS_DIR || exit_status=$?
if [ $exit_status -ne 0 ]; then
    send_slack_message "There was a problem stripping zero width spaces. Check build log!" "danger" $exit_status
fi

output=$($PYTHON3 $_CHECK_JSON_KEYS --localized-strings-dir $_LOCALIZED_STRINGS_DIR) || exit_status=$?
if [ $exit_status -ne 0 ]; then
    send_slack_message "The following files had JSON key issues:\n${output}" "danger" $exit_status
fi

$PYTHON $_GENERATE_SDF_SCRIPT --project-dir $_UNITY_PROJECT_DIR --translated-json-asset-dir $_TRANSLATED_ASSETS_DIR \
--txt-output-asset-path $_TRANSLATED_OUTPUT_FILE || exit_status=$?
if [ $exit_status -ne 0 ]; then
    send_slack_message_no_exit "There was a problem generating JP SDF from translations. Check build log!" "danger"
fi

_status=$($GIT status -s)
if [ "$_status" ]; then

    _existing_remote_branch=$($GIT ls-remote --heads $_GIT_COZMO_URI update-localized-strings-json)
    if [ "$_existing_remote_branch" ]; then
        send_slack_message "Remote update-localized-strings-json branch exists.\nDelete or master remote branch!" "danger" 1
    fi

    pushd $_TOPLEVEL
    $GIT fetch -p
    $GIT checkout master

    # delete any existing local branches other than master
    if [ $($GIT branch | wc -l) != 1 ]; then
        $GIT branch | grep -v '* master' | xargs $GIT branch -D
    fi

    $GIT config user.email "anki-smartling@anki.com"
    $GIT config user.name "anki-smartling"
    $GIT checkout -b update-localized-strings-json
    $GIT add $_LOCALIZED_STRINGS_DIR/*
    $GIT add $_ASSET_BUNDLES_DIR/*
    $GIT commit -am "Updating localized *.json from Smartling download."
    $GIT push origin update-localized-strings-json

    pr_url=$($CURL -H "Content-Type: application/json" -X POST -d \
    '{"title": "update-localized-strings-json","head": "update-localized-strings-json","base": "master"}' \
    $_GIT_COZMO_PR_URI | $PYTHON -c "import sys, json; print json.load(sys.stdin)['html_url']")

    send_slack_message "cozmo-one PR for Smartling localized strings: $pr_url" "good" 0
    popd

else
    send_slack_message "There are no new changes to commit to cozmo-one!" "warning" 1
fi
