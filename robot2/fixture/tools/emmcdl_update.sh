#!/bin/sh

#get server info/creds (sets local env vars)
. ./setcredentials.sh

#clean old files
rm $foldername.zip
rm -rf $foldername

#fetch latest and unpack
curl -L -o $foldername.zip $dblink
unzip $foldername.zip -d $foldername

#cleanup temp files
#rm $foldername.zip
