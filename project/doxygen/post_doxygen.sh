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

PYTHON3_PATH=`which python3`
if [ -n $PYTHON3_PATH ];then
  pips=`$PYTHON3_PATH -m pip list`
  if ! [[ $pips = *"requests"* ]]; then
    $PYTHON3_PATH -m pip install requests
  fi
else
  echo "Please install python3"
fi

$DOXYGEN Doxyfile

#Generate the cpp files that contain new/remove das messages
./create_changed_das_msg.py -u ngoc.b.nguyen -p L0gigear2020

cp ./das_new.cpp ./html/z_das_new.cpp
cp ./das_remove.cpp ./html/z_das_remove.cpp

#Re-run doxygen for above cpp files
$DOXYGEN Doxyfile

#./report_to_slack.py -t "Nightly DAS Documentation" -l $1
