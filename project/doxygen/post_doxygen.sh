#!/bin/bash

DEPENDENCY_LIBS=(requests beautifulsoup4 lxml)

check_and_install_lib()
{
  lib=$1
  python_path=`which python3`
  if [ -n $python_path ]; then
  pips=`$python_path -m pip list`
  if ! [[ $pips = *"$lib"* ]]; then
    $python_path -m pip install $lib
  fi
else
  echo "Please install python3"
fi
}

install_dependency_libs()
{
  dependency_libs=${DEPENDENCY_LIBS[*]}
  for lib in $dependency_libs
  do
    check_and_install_lib $lib
  done
}
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

echo "Check and install dependency libs for running DAS doxygen"
install_dependency_libs

$DOXYGEN Doxyfile

#Generate the cpp files that contain new/remove das messages
./create_changed_das_msg.py -u $2 -p $3

#Re-run doxygen for above cpp files
$DOXYGEN Doxyfile

./report_to_slack.py -t "Nightly DAS Documentation" -l $1
