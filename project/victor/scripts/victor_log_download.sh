#!/usr/bin/env bash
#
# project/victor/scripts/victor_log_download.sh
#
# Helper script to download log files from SAI blobstore.
# See also:
#    https://ankiinc.atlassian.net/wiki/spaces/SAI/pages/95125559/Blobstore+API
#
# Callers are responsible for setting up a blobstore session.
# If session is not authorized, sai-go-client will fail with an error.
#

# Verify sai-go-cli in $PATH
SAI_GO_CLI=`/usr/bin/which sai-go-cli`

if [ -z ${SAI_GO_CLI} ]; then
  echo "You must have sai-go-cli in your PATH"
  echo "Look here for link to download:"
  echo "  https://ankiinc.atlassian.net/wiki/spaces/SAI/pages/95125559/Blobstore+API"
  exit 1
fi

# Bash magic to extract filename component from blob url
id="$1"
id=${id##*/}

${SAI_GO_CLI} blobstore download --namespace qavictor --id ${id}
