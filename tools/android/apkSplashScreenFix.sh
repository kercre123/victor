#!/bin/bash

# this should be invoked by build scripts with the usage:
# apkSplashScreenFix.sh IN-APKFILE OUT-APKFILE

INFILE=$1
OUTFILE=$2

TEMPDIR=$(basename $INFILE)_temp
EDITED_APK_NAME=$(basename $INFILE)_edited.apk
EDITED_APK=$TEMPDIR/$EDITED_APK_NAME

rm -rf $TEMPDIR

echo "$INFILE before processing:" $(stat -f%z $INFILE)

# unpack the APK into $TEMPDIR
set -x
unzip -q $INFILE -d $TEMPDIR

# change the "showSplash" option from True to False
sed -i '' -e 's/showSplash">True/showSplash">False/g' $TEMPDIR/assets/bin/Data/settings.xml

# re-pack the APK file
pushd $TEMPDIR
zip -r -q $EDITED_APK_NAME . -x META-INF/\*
popd

# sign the new APK with the debug android keystore, output to $OUTFILE
jarsigner -keystore ~/.android/debug.keystore -storepass android -keypass android $EDITED_APK androiddebugkey -sigfile CERT -signedjar $OUTFILE
set +x

echo "temporary $EDITED_APK size:" $(stat -f%z $EDITED_APK)
echo "$OUTFILE after processing:" $(stat -f%z $EDITED_APK)

# remove our temporary folder
rm -rf $TEMPDIR
