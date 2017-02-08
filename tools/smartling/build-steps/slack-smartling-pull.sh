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

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi

_TOPLEVEL=`$GIT rev-parse --show-toplevel`
_SMARTLING_PULL_SCRIPT=${_TOPLEVEL}/tools/smartling/smartling-pull.sh
_LOCALIZED_STRINGS_DIR=${_TOPLEVEL}/unity/Cozmo/Assets/StreamingAssets/LocalizedStrings
_GIT_COZMO_URI=https://$ANKI_SMARTLING_ACCESS_TOKEN:x-oauth-basic@github.com/anki/cozmo-one.git

exit_status=0
$_SMARTLING_PULL_SCRIPT -t published $_LOCALIZED_STRINGS_DIR || exit_status=$?

if [ $exit_status -ne 0 ]; then
    send_slack_message "There was a problem downloading *.json from Smartling. Check build log!" "danger" $exit_status
fi

_status=$($GIT status -s)
if [ "$_status" ]; then

    _existing_remote_branch=$($GIT ls-remote --heads $_GIT_COZMO_URI update-localized-strings-json)
    if [ "$_existing_remote_branch" ]; then
        send_slack_message "Remote update-localized-strings-json branch exists.\nDelete or master remote branch!" "danger" 1
    fi

    if [ ! -f ~/.config/hub ]; then
        echo "> creating hub config file"
        mkdir -p ~/.config

        cat << EOF > ~/.config/hub
---
github.com:
- protocol: https
  user: $ANKI_SMARTLING_GIT_USER
  oauth_token: $ANKI_SMARTLING_ACCESS_TOKEN
EOF
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
    $GIT commit -am "Updating localized *.json from Smartling download."
    $GIT push origin update-localized-strings-json
    pr_url=$(hub pull-request -m 'update-localized-strings-json')

    send_slack_message "cozmo-one PR for Smartling localized strings: $pr_url" "good" 0
    popd

else
    send_slack_message "There are no new changes to commit to cozmo-one!" "warning" 1
fi
