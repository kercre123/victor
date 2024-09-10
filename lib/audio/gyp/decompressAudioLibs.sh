#!/bin/bash

SRCDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

: ${ANKI_AUDIO_SRC_DIR:="$SRCDIR/.."}

#make sure to update .gitignore when this list changes so we don't check in the unzipped files
declare -a filesToDecompress
filesToDecompress+=("wwise/libs/mac/debug/libAkSoundEngine.a")
filesToDecompress+=("wwise/libs/mac/profile/libAkSoundEngine.a")
filesToDecompress+=("wwise/libs/ios/debug/libAkSoundEngine.a")
filesToDecompress+=("wwise/libs/ios/profile/libAkSoundEngine.a")
filesToDecompress+=("wwise/libs/ios/release/libAkSoundEngine.a")

echo "[drive-audio] Decompressing Audio Libs..."
for i in "${filesToDecompress[@]}"
do
    libPath="${ANKI_AUDIO_SRC_DIR}/${i}"
    if [ ! -e "$libPath" ]; then
	gunzip -v --keep $libPath".gz"
    fi
done

exit 0
