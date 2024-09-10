#!/bin/bash
#
# Get latest SVN revision from project's generated sound bank svn repo, open Pull-Request for new clad files
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

# Get Audio & Project Repo paths
_SCRIPT_PATH=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)
pushd $_SCRIPT_PATH
_AUDIO_REPO_TOPLEVEL=`$GIT rev-parse --show-toplevel`
popd

pushd $_AUDIO_REPO_TOPLEVEL
cd ../
_PROJECT_REPO_TOPLEVEL=`$GIT rev-parse --show-toplevel`
popd

# Build script vars
_PROJECT_NAME="${_PROJECT_REPO_TOPLEVEL##*/}"
_PROJECT_EXTERNAL_PATH="${_PROJECT_REPO_TOPLEVEL}/${EXTERNAL_AUDIO_PATH}"
_PROJECT_GIT_URI=https://$ANKI_SMARTLING_ACCESS_TOKEN:x-oauth-basic@$PROJECT_GIT_URL
_UPDATE_AUDIO_ASSETS_SCRIPT=${_PROJECT_REPO_TOPLEVEL}/tools/audio/UpdateAudioAssets.py
_GIT_BRANCH_NAME=update-audio-assets
_GIT_USERNAME="anki-smartling"
_GIT_EMAIL="anki-smartling@anki.com"

echo "_SCRIPT_PATH: ${_SCRIPT_PATH}"
echo "SVN_ASSET_REPO: ${SVN_ASSET_REPO}"
echo "EXTERNAL_AUDIO_PATH: ${EXTERNAL_AUDIO_PATH}"
echo "_AUDIO_REPO_TOPLEVEL: ${_AUDIO_REPO_TOPLEVEL}"
echo "_PROJECT_REPO_TOPLEVEL: ${_PROJECT_REPO_TOPLEVEL}"
echo "_PROJECT_NAME: ${_PROJECT_NAME}"
echo "_PROJECT_EXTERNAL_PATH: ${_PROJECT_EXTERNAL_PATH}"
echo "_PROJECT_GIT_URI: ${_PROJECT_GIT_URI}"
echo "_UPDATE_AUDIO_ASSETS_SCRIPT: ${_UPDATE_AUDIO_ASSETS_SCRIPT}"
echo "_GIT_BRANCH_NAME: ${_GIT_BRANCH_NAME}"
echo "_GIT_EMAIL: ${_GIT_EMAIL}"

# remove any existing downloads
rm -rf "$_PROJECT_EXTERNAL_PATH"
mkdir -p "$_PROJECT_EXTERNAL_PATH"

# Get previous & current SVN revisions
exit_status=0
svn_rev=$(svn info $SVN_ASSET_REPO --username $SVN_USERNAME --password $SVN_PASSWORD | grep 'Last Changed Rev' | awk '{ print $4; }')
prev_svn_rev=$(python $_UPDATE_AUDIO_ASSETS_SCRIPT current-version | grep 'version: '| cut -d " " -f 2) || exit_status=$?

echo "UpdateAudioAssets.py current-version: ${prev_svn_rev}"
echo "svn_rev: ${svn_rev}"

# Get logs for Git comment
svn_logs=$(svn log -r $prev_svn_rev:$svn_rev $SVN_ASSET_REPO --username $SVN_USERNAME --password $SVN_PASSWORD)

# Run scripts to update Audio Assets and Generate defines
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

# Create GIT commit
pushd ${_PROJECT_REPO_TOPLEVEL}
_status=$($GIT status -s)
if [ "$_status" ]; then

    _existing_remote_branch=$($GIT ls-remote --heads $_PROJECT_GIT_URI $_GIT_BRANCH_NAME)
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

    $GIT config remote.origin.url $_PROJECT_GIT_URI
    $GIT config user.email $_GIT_EMAIL
    $GIT config user.name $_GIT_USERNAME
    $GIT checkout -b $_GIT_BRANCH_NAME
    $GIT commit -am "Updating Audio Assets v${svn_rev}" -m"${svn_logs}"
    $GIT push origin $_GIT_BRANCH_NAME
    pr_url=$(hub pull-request -m $_GIT_BRANCH_NAME)

    send_slack_message_and_exit "$_PROJECT_NAME PR for updated audio assets: $pr_url" "good" 0

else
    send_slack_message_and_exit "There are no new changes to commit to $_PROJECT_NAME!" "warning" 1
fi
popd
