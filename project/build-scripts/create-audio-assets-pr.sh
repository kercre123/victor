#!/bin/bash
#
# Get latest SVN revision from cozmosoundbanks, open Pull-Request for new clad files
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

    echo "$_payload" > tmp$$.json
    curl -X POST --data-urlencode payload@tmp$$.json $SLACK_TOKEN_URL
    rm tmp$$.json
}

send_slack_message_and_exit() {
    send_slack_message "$1" "$2"
    exit $3
}

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi

_SCRIPT_PATH=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)
_TOPLEVEL_COZMO=${_SCRIPT_PATH}/../../
_GIT_COZMO_URI=https://$ANKI_SMARTLING_ACCESS_TOKEN:x-oauth-basic@github.com/anki/cozmo-one.git
_UPDATE_AUDIO_ASSETS_SCRIPT=${_TOPLEVEL_COZMO}/tools/audio/UpdateAudioAssets.py
_SVN_COZMOSOUNDBANKS_REPO=https://svn.ankicore.com/svn/cozmosoundbanks/trunk
_EXTERNALS_DIR=cozmo-one/EXTERNALS
_SOUNDBANK_DIR=${_EXTERNALS_DIR}/cozmosoundbanks/GeneratedSoundBanks
_GIT_BRANCH_NAME=update-audio-assets
_GIT_USERNAME="anki-smartling"
_GIT_EMAIL="anki-smartling@anki.com"

# remove any existing downloads
rm -rf $_EXTERNALS_DIR
mkdir -p $_SOUNDBANK_DIR

svn_rev=$(svn info $_SVN_COZMOSOUNDBANKS_REPO --username $SVN_USERNAME --password $SVN_PASSWORD | grep 'Last Changed Rev' | awk '{ print $4; }')

exit_status=0
output=$(python $_UPDATE_AUDIO_ASSETS_SCRIPT update $svn_rev) || exit_status=$?
echo "UpdateAudioAssets.py update output: ${output}"
if [ $exit_status -ne 0 ]; then
    send_slack_message_and_exit "There was a problem getting r${svn_rev} audio assets.\n${output}" "danger" $exit_status
else
    send_slack_message "UpdateAudioAssets.py update output: ${output}" "warning"
fi

output=$(python $_UPDATE_AUDIO_ASSETS_SCRIPT generate) || exit_status=$?
echo "UpdateAudioAssets.py generate output: ${output}"
if [ $exit_status -ne 0 ]; then
    send_slack_message_and_exit "There was a problem generating CLAD files.\n${output}" "danger" $exit_status
else
    send_slack_message "UpdateAudioAssets.py generate output: ${output}" "warning"
fi

pushd $_TOPLEVEL_COZMO
_status=$($GIT status -s)
if [ "$_status" ]; then

    _existing_remote_branch=$($GIT ls-remote --heads $_GIT_COZMO_URI $_GIT_BRANCH_NAME)
    if [ "$_existing_remote_branch" ]; then
        send_slack_message_and_exit "Remote ${_GIT_BRANCH_NAME} branch exists.\nDelete or master remote branch!" "danger" 1
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

    $GIT fetch -p
    $GIT checkout master

    # delete any existing local branches other than master
    if [ $($GIT branch | wc -l) != 1 ]; then
        $GIT branch | grep -v '* master' | xargs $GIT branch -D
    fi

    $GIT config remote.origin.url $_GIT_COZMO_URI
    $GIT config user.email $_GIT_EMAIL
    $GIT config user.name $_GIT_USERNAME
    $GIT checkout -b $_GIT_BRANCH_NAME
    $GIT commit -am "Updating Audio Assets v${svn_rev}"
    $GIT push origin $_GIT_BRANCH_NAME
    pr_url=$(hub pull-request -m $_GIT_BRANCH_NAME)

    send_slack_message_and_exit "cozmo-one PR for updated audio assets: $pr_url" "good" 0

else
    send_slack_message_and_exit "There are no new changes to commit to cozmo-one!" "warning" 1
fi
popd
