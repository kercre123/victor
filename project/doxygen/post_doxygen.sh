#!/bin/bash

DOXYGEN=`which doxygen`
if [ -z $DOXYGEN ];then

  BREW=`which brew`
  APT_GET=`which apt-get`
  if [ -n $BREW ];then
    $BREW install doxygen
  elif [ -n $APT_GET ];then
    sudo $APT_GET install doxygen
  else
    echo "Please install doxygen"
    exit 1
  fi
DOXYGEN=`which doxygen`

fi

$DOXYGEN Doxyfile

#Generate the cpp files that contain new/remove das messages
./create_changed_das_msg.py -u $2 -p $3

#Re-run doxygen for above cpp files
$DOXYGEN Doxyfile

./report_to_slack.py -t "Nightly DAS Documentation" -l $1
