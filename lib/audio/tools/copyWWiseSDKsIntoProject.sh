#!/bin/bash

# This scrip will copy and configure  Wwise include headers and SDK files
# needed for Mac OS X, iOS and Android platforms. Pass the platform include
# and SDK directories in as arguments. This scrip copies the include headers,
# runs “lipo” on iOS and iOS simulator libs, copies all the libs and finally
# compresses files over 100 MB for GitHub check in.

if [ "$#" -ne 5 ]; then
	echo "Incorrect # of parameters. Must pass in the Mac/iOS Include directory, Android Include directory, Mac_SDK directory, iOS_SDK directory & Android_SDK directory file locations"
fi

# Error out
set -e

SRC_DIR=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))

PROJECT_INCLUDES_DIR="$SRC_DIR/../wwise/includes"
PROJECT_LIBS_DIR="$SRC_DIR/../wwise/libs"

INCLUDE_MAC_IOS=$1
INCLUDE_ANDROID=$2
MAC_SDK=$3
IOS_SDK=$4
ANDROID_SDK=$5

echo ""
echo $PROJECT_INCLUDES_DIR
echo $PROJECT_LIBS_DIR
echo ""
echo "Mac & iOS Include header Path: $INCLUDE_MAC_IOS"
echo "Android Include header Path: $INCLUDE_ANDROID"
echo "Mac SDK Path: $MAC_SDK"
echo "iOS SDK Path: $IOS_SDK"
echo "Android SDK Path: $ANDROID_SDK"
echo ""

#Rsync files
# arg 1 - Source dir
# arg 2 - Destination dir
function RSyncDir {
	# check source file
	if [ -d "$1" ]; then
		# Create destination if needed
		if [ ! -d "$2" ]; then
			mkdir -p "$2"
		fi
			echo "RSync $1 to $2"
			rsync -rlpt --delete "$1" "$2"
	else
		echo "Can NOT RSync $1"
		exit 1
	fi
	
}


# RSync all build modes debug, profile $ release libs
# arg 1 - SDK dir
# arg 2 - Destination Lib dir
function RSyncPlatformLibs {
	if [ -d "$1" ]; then
		echo "Found $1!!!"
		PLATFORM_LIB_DIR="$2"
		
		#Debug
		DEBUG="$1/Debug/lib/"
		RSyncDir "$DEBUG" "$2/debug/"
		
		#Profile
		PROFILE="$1/Profile/lib/"
		RSyncDir "$PROFILE" "$2/profile/"

		#Release
		RELEASE="$1/Release/lib/"
		RSyncDir "$RELEASE" "$2/release/"

	else
		echo "$1 Does NOT exist"
		exit 1
	fi
}


# Create iOS libs by lipo os & simulator libs together
# arg 1 - iOS SDK dir
# arg 2 - Build Mode
# arg 3 - Result dir
function CreateIOSAndSimulatorLipos {
	if [ -d "$1" ]; then
		
		BUILD_MODE=$2
		OS_POSTFIX="-iphoneos"
		SIMULATOR_POSTFIX="-iphonesimulator"
		IOS_DIR="$1/$BUILD_MODE$OS_POSTFIX/lib"
		SIMULATOR_DIR="$1/$BUILD_MODE$SIMULATOR_POSTFIX/lib"
		RESULT_DIR=$3
		
		echo "\"lipo\" iOS and simulator \"$BUILD_MODE\" libs together"
		echo "iOS - $IOS_DIR"
		echo "Simulator - $SIMULATOR_DIR"
		echo "Temp dir = $RESULT_DIR"
		
		mkdir -p $RESULT_DIR
		
		if [ -d "$IOS_DIR" -a -d "$SIMULATOR_DIR" ]; then
			echo "iOS & Simulator dir exist for BuildMode: $BUILD_MODE"
			
			# loop through libs createing lipo
			for file in "$IOS_DIR"/*
			do
				FILE_NAME=${file##*/}
				SIMULATOR_FILE="$SIMULATOR_DIR/$FILE_NAME"
				OUTPUT_FILE="$RESULT_DIR/$FILE_NAME"
				
				if [ -e "$SIMULATOR_FILE" ]; then
					lipo -create "$file" "$SIMULATOR_FILE" -output "$OUTPUT_FILE"
				else
					echo "Simulator $SIMULATOR_FILE lib Does NOT exist"
					exit 1
				fi
				
			done		
		else
			echo "iOS & Simulator dir do NOT exist for BuildMode: $BUILD_MODE"
		fi
		echo ""
		
	else
		echo "Can NOT \"lipo\" iOS and simulator libs together"
		exit 1
	fi
}


# Merge Mac, iOS & Android include files
INCLUE_TEMP_ROOT_DIR="$SRC_DIR/include_temp"
echo "Merge Mac, iOS & Android include files"
mkdir "$INCLUE_TEMP_ROOT_DIR"
echo "Copy Mac & iOS include files"
cp -r "$INCLUDE_MAC_IOS/" "$INCLUE_TEMP_ROOT_DIR/"
echo "Copy Android include files"
cp -r "$INCLUDE_ANDROID/" "$INCLUE_TEMP_ROOT_DIR/"

# RSync all headder files
echo "RSync WWise Include Mac, iOS & Android Files ..."
RSyncDir "$INCLUE_TEMP_ROOT_DIR/" "$PROJECT_INCLUDES_DIR/"

# Cleanup temp files
echo "Clean up include temp files ..."
rm -rf -- "$INCLUE_TEMP_ROOT_DIR"
echo ""


# iOS
# Run lipo on all the iOS & Simulator libs
LIPO_TEMP_ROOT_DIR="$SRC_DIR/lipo_temp"
echo "Run lipo on all iOS & Simulator Libs ..."

BUILD_MODES=("Debug" "Profile" "Release")
for mode in ${BUILD_MODES[@]}; do
	
	RESULT_DIR="$LIPO_TEMP_ROOT_DIR/$mode/lib"
	mkdir -p "$RESULT_DIR"
	CreateIOSAndSimulatorLipos "$IOS_SDK" "$mode" "$RESULT_DIR"
	
done


# Copy (RSync) all Platform Libs into project
PLATFORMS=("mac" "ios" "android")
SDK_LIBS=("$MAC_SDK" "$LIPO_TEMP_ROOT_DIR" "$ANDROID_SDK")
IDX=0

for platform in ${PLATFORMS[@]}; do
	
	echo "RSycn $platform Libs ..."
	SDK=${SDK_LIBS[$IDX]}
	RSyncPlatformLibs "$SDK" "$PROJECT_LIBS_DIR/$platform"
	echo ""
	let IDX=IDX+1
	
done


# Cleanup temp files
echo "Clean up lipo temp files ..."
rm -rf -- $LIPO_TEMP_ROOT_DIR

echo ""
echo "Completed Successfully!"
echo ""

# SDK Notes
echo "SDK Notes:"
echo "Remember to update the AK_MAX_PATH define to allow longer paths string. Update the define in \"wwise/includes/AK/SoundEngine/Platforms/POSIX/AkTypes.h\" & \"wwise/includes/AK/SoundEngine/Platforms/Windows/AkTypes.h\""
echo "Change \"#define AK_MAX_PATH  260\" -> \"#define AK_MAX_PATH  512\""
echo ""

exit 0
