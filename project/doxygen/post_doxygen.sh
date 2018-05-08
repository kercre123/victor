#!/bin/bash

DOXYGEN=`which doxygen`
if [ -z $DOXYGEN ];then

  BREW=`which brew`
  APT_GET=`which apt-get`
  if [ -z $BREW ];then
    $BREW install doxygen
  elif [ -z $APT_GET ];then
    sudo $APT_GET install doxygen
  else
    echo "Please install doxygen"
    exit 1
  fi
DOXYGEN=`which doxygen`

fi

$DOXYGEN Doxyfile
./report_to_slack.py -t "Nightly DAS Documentation" -l $1
