#!/usr/bin/env bash

ERROR_MSG=$1
shift
if eval $@; then
  exit 0
else
  >&2 echo $ERROR_MSG
  exit 1
fi
