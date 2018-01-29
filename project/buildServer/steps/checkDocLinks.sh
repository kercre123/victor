#!/usr/bin/env bash

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi

: ${TOPLEVEL:=`$GIT rev-parse --show-toplevel`}

${TOPLEVEL}/tools/brokenLinkCheck/testMarkdownLinks.py ${TOPLEVEL}/tools/brokenLinkCheck/testMarkdownLinks_config.json
EXIT_STATUS=$?

exit $EXIT_STATUS
