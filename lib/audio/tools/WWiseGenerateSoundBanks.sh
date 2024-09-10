#!/bin/bash

# This scrip will generate sound banks and Tar them to the SVN folder.
# I'm assuming the WWise Application is in the Application folder.


# Error out
set -e
set -u

_SCRIPT_PATH=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)
_SCRIPT_NAME=`basename "$0"`

function usage() {
	echo ""
	echo "$_SCRIPT_NAME [-s WWISE_PROJECT_DIR] [-o OUTPUT_DIR] [-v] [-e] [-C]"
	echo "	-s		: WWise Project directory "
	echo "	-o		: SVN destination directory"
	echo "	-v		: verbose output (optional)"
	echo " 	-e		: Continue on Error (optional)"
	echo "	-C		: Clean Cached Audio Files - Regenerate all audio files (optional)"
	echo ""
}

# Check that WWise exist
function validateApp() {
	if [ -x  $1 ]; then
		echo "Found WWise App - \"$1\""
	else
		echo "Can NOT find WWise App - \"$1\""
		exit 1
	fi
}


WWISE_APP_LOCATION="/Applications/Wwise.app/Contents/MacOS/WwiseCLI.sh"
SOURCE_DIR=""
OUTPUT_DIR=""
VERBOSE=0
CLEAN=0
CONTINUE_ON_ERROR=0

# Run Script
validateApp $WWISE_APP_LOCATION

while getopts ":s:o:vC" opt; do
    case $opt in
        s)
            SOURCE_DIR="$OPTARG"
            ;;
        o)
			OUTPUT_DIR="$OPTARG"
            ;;
        v)
            VERBOSE=1
            ;;
		e)
			CONTINUE_ON_ERROR=1
			;;
        C)
            CLEAN=1
            ;;
        :)
            echo "Option -$OPTARG requires an argument." >&2
            usage
            exit 1
            ;;
    esac
done

# Check Source Dir
if [ -z $SOURCE_DIR ]; then
	echo "Missing WWise Project Directory parameter"
	usage
	exit 1
else
	if [ ! -d $SOURCE_DIR ]; then
		echo "Must Provide a valid WWise Project Directory"
		usage
		exit 1
	fi
fi

# Check Output Dir
if [ -z $OUTPUT_DIR ]; then
       echo "Must Provide an Output Directory"
       usage
       exit 1
fi

# Find Wwise project File
PROJECT_FILE=`find $SOURCE_DIR -name "*.wproj"`

if [ ! -f  $PROJECT_FILE ]; then
	echo "Can NOT find WWise Project file!!!"
	exit 1
fi

# Generate WWise Command
GENERATED_COMMAND="$WWISE_APP_LOCATION $PROJECT_FILE -GenerateSoundBanks"

# Add Clean flag
if [ $CLEAN -eq 1 ]; then
	GENERATED_COMMAND+=" -ClearAudioFileCache"
fi

# Add -Verbose logging
if [ $VERBOSE -eq 1 ]; then
	GENERATED_COMMAND+=" -Verbose"
fi

# Add -ContinueOnError argument
if [ $CONTINUE_ON_ERROR -eq 1 ]; then
	GENERATED_COMMAND+=" -ContinueOnError"
fi

echo "Running Command: $GENERATED_COMMAND"
echo ""

# Don't error out if WWise returns 1 or 2
set +e
eval $GENERATED_COMMAND
set -e

# Handle Result
RESULT=$?
if [ $RESULT -eq 0 ]; then
	echo "Generated Sound Banks Successful!!"
	
elif [ $RESULT -eq 2 ]; then
	echo "Generated Sound Banks with warnings!!"

else	
	echo "FAILED to Generated Sound Banks!!"
	if [ CONTINUE_ON_ERROR = 0 ]; then
		exit 1
	fi
fi

# check source file
GENERATED_SOUND_FOLDER="GeneratedSoundBanks"
GENERATED_SOUND_DIR="$SOURCE_DIR/$GENERATED_SOUND_FOLDER"
if [ ! -d $GENERATED_SOUND_DIR ]; then
	echo "Can NOT find Generated Sound Bank directory - $GENERATED_SOUND_DIR"
	exit 1;
fi

# Create destination if needed
if [ ! -d $OUTPUT_DIR ]; then
	echo "Need to Create Output directory $OUTPUT_DIR"
	mkdir -p $OUTPUT_DIR
fi

#QZip Generated Sounds 
FILENAME=GeneratedSoundBanks.tar.gz
echo "Tar $GENERATED_SOUND_DIR to $OUTPUT_DIR/$FILENAME"
pushd $SOURCE_DIR
tar -zcvf $OUTPUT_DIR/$FILENAME $GENERATED_SOUND_FOLDER
popd

echo ""
echo "Completed Successfully!"
echo ""

exit 0
