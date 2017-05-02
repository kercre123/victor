#!/bin/bash

# this should be invoked by build scripts with the usage:
# apkSplashScreenFix.sh IN-APKFILE OUT-APKFILE

INFILE=$1
OUTFILE=$2

APK_FILE=$(basename $OUTFILE)
SETTINGS_FILE=assets/bin/Data/settings.xml

echo "$INFILE before processing:" $(stat -f%z $INFILE)

# unpack the APK into $TEMPDIR
set -x
cp $INFILE $OUTFILE
pushd $(dirname $OUTFILE)

# extract settings.xml
unzip -o $APK_FILE $SETTINGS_FILE
# change the "showSplash" option from True to False
sed -i '' -e 's/showSplash">True/showSplash">False/g' $SETTINGS_FILE
# put our modified file back in the apk
zip -u $APK_FILE $SETTINGS_FILE

# remove META-INF folder from zip (jarsigner will replace it and requires it not be there)
zip -d $APK_FILE META-INF/\*

# sign the new APK with the debug android keystore, output to $OUTFILE
jarsigner -keystore ~/.android/debug.keystore -storepass android -keypass android $APK_FILE androiddebugkey -sigfile CERT -signedjar $APK_FILE
set +x

echo "$OUTFILE after processing:" $(stat -f%z $OUTFILE)
popd
