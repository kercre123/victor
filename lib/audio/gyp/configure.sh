#!/bin/bash

function arg_help() {
  echo "Usage: $0 [-h] [-p {ios/mac}] [-f OUTPUT_DIR] "
}

# Parse Arguments
OUTPUT_PATH=../generated
PLATFORM=
while getopts "ho:p:" opt; do

  case $opt in
    h)
      arg_help
      exit 0
      ;;
      
    o)
      OUTPUT_PATH=$OPTARG
      ;;
      
    p)
      if [ $OPTARG == "ios" ]; then
        PLATFORM=$OPTARG
        
      elif [ $OPTARG == "mac" ]; then
        PLATFORM=$OPTARG
        
      else
        echo "Invalid Platform $OPTARG"
        arg_help
        exit 0
      
      fi
      
      echo "Geneate project for $PLATFORM platform"
      ;;
    
    \?)
      echo "Invalid option: -$OPTARG" >&2
      ;;  
      
    :)
      echo "Option -$OPTARG requires an argument." >&2
      ;;
      
   esac
done
 

function gyp_help() {
  echo "to install gyp:"
  echo "cd ~/your_workspace/"
  echo "git clone git@github.com:anki/anki-gyp.git"
  echo "echo PATH=$HOME/your_workspace/gyp:\$PATH >> ~/.bash_profile"
  echo ". ~/.bash_profile"
}

#location of this script
SRCDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $SRCDIR

#root of the git project
GIT=`which git`
if [ -z $GIT ]; then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

GYP=
if [ ! -z $ANKI_BUILD_GYP_PATH ]; then
  GYP="${ANKI_BUILD_GYP_PATH}/gyp"
fi

: ${GYP:=`which gyp`}
if [ -z $GYP ]; then
  echo gyp not found
  gyp_help
  exit 1
  echo $GYP
fi

$GYP --version | grep ANKI > /dev/null 2>&1

if [ $? -ne 0 ]; then
  echo incorrect version of gyp installed
  gyp_help
  exit 1
fi

# Run script to download current WWise SDK libs
../wwise/fetch-wwise.sh

: ${WWISE_SDK_ROOT:="${ANKI_WWISE_SDK_ROOT}"}
: ${WWISE_SDK_ROOT:="../wwise/versions/current"}


# Determin what projects to build
if [ -z $PLATFORM ]; then
  # Generate both platforms
  # mac
  #################### GYP_DEFINES ####    
  DEFINES=" audio_library_type=static_library
            audio_library_build=profile
            wwise_sdk_root='${WWISE_SDK_ROOT}'
            OS=mac
          "
  
  GYP_DEFINES=${DEFINES} ${GYP} audioEngine.gyp --check --depth . -f xcode --generator-output="$OUTPUT_PATH/mac"
  
  # ios
  #################### GYP_DEFINES ####
  DEFINES=" audio_library_type=static_library
            audio_library_build=profile
            wwise_sdk_root='${WWISE_SDK_ROOT}'
            OS=ios
          "
  GYP_DEFINES=${DEFINES} ${GYP} audioEngine.gyp --check --depth . -f xcode --generator-output="$OUTPUT_PATH/ios"
  
  
elif [ $PLATFORM == "mac" ]; then
  # Generate only mac platform
  #################### GYP_DEFINES ####    
  DEFINES=" audio_library_type=static_library
            audio_library_build=profile
            wwise_sdk_root='${WWISE_SDK_ROOT}'
            OS=mac
          "
  GYP_DEFINES=${DEFINES} ${GYP} audioEngine.gyp --check --depth . -f xcode --generator-output="$OUTPUT_PATH"
  
elif [ $PLATFORM == "ios" ]; then
  # Generate only ios platform
  #################### GYP_DEFINES ####
  DEFINES=" audio_library_type=static_library
            audio_library_build=profile
            wwise_sdk_root='${WWISE_SDK_ROOT}'
            OS=ios
          "
  GYP_DEFINES=${DEFINES} ${GYP} audioEngine.gyp --check --depth . -f xcode --generator-output="$OUTPUT_PATH"
fi
    
