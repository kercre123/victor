#!/bin/bash

SRCDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

#make sure to update .gitignore when this list changes so we don't check in the unzipped files
declare -a filesToCompress
filesToCompress+=("$SRCDIR/../wwise/libs/mac/debug/libAkSoundEngine.a")
filesToCompress+=("$SRCDIR/../wwise/libs/mac/profile/libAkSoundEngine.a")
filesToCompress+=("$SRCDIR/../wwise/libs/ios/debug/libAkSoundEngine.a")
filesToCompress+=("$SRCDIR/../wwise/libs/ios/profile/libAkSoundEngine.a")
filesToCompress+=("$SRCDIR/../wwise/libs/ios/release/libAkSoundEngine.a")

echo [Compress Audio Libs]
for i in "${filesToCompress[@]}"
do
    if [ -e $i ]; then
		
		gzip -vf --keep $i 
		#$i".gz"
	
    fi
done

exit 0
