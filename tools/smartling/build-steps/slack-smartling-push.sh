#!/bin/bash
#
# Upload strings files to smartling using buildbot
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

_SCRIPT_PATH=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)
_SMARTLING_PUSH_SCRIPT=${_SCRIPT_PATH}/../smartling-push.sh

exit_status=0
$_SMARTLING_PUSH_SCRIPT || exit_status=$?

if [ $exit_status -eq 0 ]; then
    send_slack_message "Smartling cozmo-one *.json were sent successfully." "good" $exit_status
else
    send_slack_message "There was a problem uploading en-US/*.json to Smartling. Check build log!" "danger" $exit_status
fi
