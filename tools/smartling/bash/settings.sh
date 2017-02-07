#!/bin/bash

# change this value to your "Project Id" found on the Project Settings -> API page in dashboard

: ${ANKI_BUILD_SMARTLING_PROJECT_ID:="48506f3eb"}
PROJECT_ID=$ANKI_BUILD_SMARTLING_PROJECT_ID

# change this value to your "Smartling API key" found on the Project Settings -> API page in dashboard
# For Cozmo, this value is stored in Dropbox, available to all Engineers and employees

# check home dir
SMARTLING_DOTFILE="${HOME}/.smartling.sh"

# check Dropbox
ANKI_DROPBOX="${HOME}/Dropbox\ \(Anki,\ Inc\)"
SMARTLING_DROPBOX="${ANKI_DROPBOX}/Anki\ Software\ Engineering/dotfiles/cozmo/smartling.sh"

[ -z ${ANKI_BUILD_SMARTLING_API_KEY} ] && [ -f "${SMARTLING_DOTFILE}" ] && source ${SMARTLING_DOTFILE}
[ -z ${ANKI_BUILD_SMARTLING_API_KEY} ] && [ -f "${SMARTLING_DROPBOX}" ] && source ${SMARTLING_DROPBOX}
API_KEY=$ANKI_BUILD_SMARTLING_API_KEY

if [ -z ${API_KEY} ]; then
  echo "API_KEY for smartling API is not set."
  echo ""
  echo "Check that ANKI_BUILD_SMARTLING_API_KEY is exported in your shell"
  echo "or available to smartling commands."
  echo ""
  echo "To configure your Cozmo environment to work with smartling:"
  echo " - Sync the env file via Dropbox to:"
  echo "   ${SMARTLING_DROPBOX}"
  echo " OR"
  echo " - Download smartling.sh from Dropbox:"
  echo "   https://www.dropbox.com/s/svfnm0jbb3gldcu/smartling.sh?raw=1"
  echo " - Save it to ~/.smartling.sh"
fi

# the File API URL to use
API_SERVICE_URL=api.smartling.com # production
#API_SERVICE_URL=sandbox-api.smartling.com # sandbox

# the File API version to use
API_VERSION='v1'

