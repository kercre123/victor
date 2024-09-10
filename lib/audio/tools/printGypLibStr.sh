#!/bin/bash

MAC_IOS_LIB_DIR=$1
ANDROID_LIB_DIR=$2

echo "Mac/iOS GYP"
echo ""
# Print Gyp Includes
for file in $MAC_IOS_LIB_DIR/*
do
	fileName=${file##*/}
	echo "'$fileName',"

done

echo ""
echo ""
echo ""

Echo "Android GYP"
echo ""
# Print Gyp Includes
for file in $ANDROID_LIB_DIR/*
do
	fileName=${file##*/}
	subString=${fileName:3}
	subString=`echo $subString | sed 's/\.a$//'`
	echo "'-l$subString',"

done

echo ""
echo ""
echo ""
echo "Completed!"
echo ""
