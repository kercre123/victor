#!/bin/bash
#
# Scans all codelab featured projects, and adds any strings that look localizeable to the english localization string list
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

_STREAMING_ASSETS_DIR=${_TOPLEVEL}/unity/Cozmo/Assets/StreamingAssets
_TRANSLATE_CODELAB_SCRIPT=${_TOPLEVEL}/tools/scratch/autotranslate/localize_featured_projects.py
_TRANSLATE_CODELAB_BLACKLIST=${_TOPLEVEL}/tools/scratch/autotranslate/blacklist.json
_TRANSLATED_CODELAB_PROJECT_CONFIG_FILE=${_STREAMING_ASSETS_DIR}/Scratch/featured-projects.json
_TRANSLATED_CODELAB_PROJECTS_DIR=${_STREAMING_ASSETS_DIR}/Scratch/featuredProjects
_ENGLISH_LOCALIZATION_STRINGS_FILE=${_STREAMING_ASSETS_DIR}/LocalizedStrings/en-US/CodeLabFeaturedContentStrings.json
_SMARTLING_PARSE_PR_RESPONSE_SCRIPT=${_TOPLEVEL}/tools/smartling/smartling-parse-pr-response.py

_GIT_BRANCH_NAME=update-localization-codelab-english-json
_GIT_COZMO_URI=https://$ANKI_SMARTLING_ACCESS_TOKEN:x-oauth-basic@github.com/anki/cozmo-one.git
_GIT_COZMO_PR_URI=https://$ANKI_SMARTLING_ACCESS_TOKEN:x-oauth-basic@api.github.com/repos/anki/cozmo-one/pulls

exit_status=0
$PYTHON3 $_TRANSLATE_CODELAB_SCRIPT -as -b $_TRANSLATE_CODELAB_BLACKLIST --streaming_assets_path $_STREAMING_ASSETS_DIR --project_folder $_TRANSLATED_CODELAB_PROJECTS_DIR --project_config_file $_TRANSLATED_CODELAB_PROJECT_CONFIG_FILE --scan_loc_target $_ENGLISH_LOCALIZATION_STRINGS_FILE || exit_status=$?
if [ $exit_status -ne 0 ]; then
    send_slack_message "There was a problem scanning featured projects. Check build log!" "danger" $exit_status
fi

_status=$($GIT status -s)
if [ "$_status" ]; then

    _existing_remote_branch=$($GIT ls-remote --heads $_GIT_COZMO_URI $_GIT_BRANCH_NAME)
    if [ "$_existing_remote_branch" ]; then
        send_slack_message "Remote $_GIT_BRANCH_NAME branch exists.\nDelete or master remote branch!" "danger" 1
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
    $GIT checkout -b $_GIT_BRANCH_NAME
    $GIT add $_ENGLISH_LOCALIZATION_STRINGS_FILE
    $GIT commit -am "Updating english CodeLabFeaturedContentStrings.json"
    $GIT push origin $_GIT_BRANCH_NAME

    pr_url=$($CURL -H "Content-Type: application/json" -X POST -d \
    '{"title": "update-localization-codelab-english-json","head": "update-localization-codelab-english-json","base": "master"}' \
    $_GIT_COZMO_PR_URI | $PYTHON3 $_SMARTLING_PARSE_PR_RESPONSE_SCRIPT)

    send_slack_message "cozmo-one PR for Smartling localized strings: $pr_url" "good" 0
    popd

else
    send_slack_message "There are no new changes to commit to cozmo-one!" "warning" 1
fi
