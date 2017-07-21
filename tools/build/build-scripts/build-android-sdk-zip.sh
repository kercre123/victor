#!/usr/bin/env bash

set -e

die ( ) {
    echo
    echo "$*"
    echo
    exit 1
}

# Determine the Java command to use to start the JVM.
if [ -n "$JAVA_HOME" ] ; then
    if [ -x "$JAVA_HOME/jre/sh/java" ] ; then
        # IBM's JDK on AIX uses strange locations for the executables
        JAVACMD="$JAVA_HOME/jre/sh/java"
    else
        JAVACMD="$JAVA_HOME/bin/java"
    fi
    if [ ! -x "$JAVACMD" ] ; then
        die "ERROR: JAVA_HOME is set to an invalid directory: $JAVA_HOME

Please set the JAVA_HOME variable in your environment to match the
location of your Java installation."
    fi
else
    JAVACMD="java"
    which java >/dev/null 2>&1 || die "ERROR: JAVA_HOME is not set and no 'java' command could be found in your PATH.

Please set the JAVA_HOME variable in your environment to match the
location of your Java installation."
fi

# Show all the java settings to help with debugging execution failures
$JAVACMD -XshowSettings:all -version

# Determine the ant command to use
if [ -n "$ANT_HOME" ]; then
    ANT="$ANT_HOME/bin/ant"
    if [ ! -x "$ANT" ] ; then
	die "ERROR: ANT_HOME is set to an invalid directory: $ANT_HOME

Please set the ANT_HOME variable in your environment to match the
location of your ant installation."
    fi
else
    ANT="ant"
    which ant >/dev/null 2>&1 || die "ERROR: ANT_HOME is not set and no 'ant' command could be found in your PATH.

Please set the ANT_HOME variable in your environment to match the
location of your ant installation."
fi

set -u

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
TOPLEVEL=`pushd ${SCRIPT_PATH}/.. >> /dev/null; pwd; popd >> /dev/null`

HOST_PLATFORM=`uname -a | awk '{print tolower($1);}' | sed -e 's/darwin/macosx/'`
: ${REPO_OS_OVERRIDE:=$HOST_PLATFORM}

ANKI_ANDROID_SDK_VERSION=r3

TOOLS_VERSION=r25.2.3

BUCK_VERSION=v2017.05.09.01

PACKAGES=" \
\"build-tools;23.0.3\" \
\"platforms;android-23\" \
\"extras;google;m2repository\" \
\"extras;android;m2repository\" \
\"platform-tools\" \
"

BUILD_DIR=${TOPLEVEL}/build-android-sdk
DIST_DIR=${BUILD_DIR}/dist
ANKI_ANDROID_SDK_NAME=android-sdk-${REPO_OS_OVERRIDE}-anki-${ANKI_ANDROID_SDK_VERSION}
ANKI_ANDROID_SDK_DIR=${BUILD_DIR}/${ANKI_ANDROID_SDK_NAME}

rm -rf ${BUILD_DIR}
mkdir -p ${ANKI_ANDROID_SDK_DIR}

TOOLS_ZIP_NAME=tools_${TOOLS_VERSION}-${REPO_OS_OVERRIDE}.zip
TOOLS_ZIP_PATH=${ANKI_ANDROID_SDK_DIR}/${TOOLS_ZIP_NAME}
TOOLS_URL=https://dl.google.com/android/repository/${TOOLS_ZIP_NAME}

echo "Downloading ${TOOLS_URL} ..."
curl -o ${TOOLS_ZIP_PATH} ${TOOLS_URL}

echo "Unzipping ${TOOLS_ZIP_PATH} ..."
unzip -q -d ${ANKI_ANDROID_SDK_DIR} ${TOOLS_ZIP_PATH}
rm ${TOOLS_ZIP_PATH}

export REPO_OS_OVERRIDE
echo "Installing SDK packages could take more than 10 minutes depending on Internet speed"
/usr/bin/env expect << EOS
set timeout -1
spawn ${ANKI_ANDROID_SDK_DIR}/tools/bin/sdkmanager ${PACKAGES}
expect "Accept? (y/N):"
send "y\r"
expect eof
EOS

mkdir -p ${ANKI_ANDROID_SDK_DIR}/anki/bin

# Build buck
pushd ${BUILD_DIR}
git clone https://github.com/facebook/buck.git
pushd buck
git checkout ${BUCK_VERSION}
$ANT
./bin/buck build buck
cp -pv `./bin/buck targets --show_output buck | awk '{print $2;}'` \
   ${ANKI_ANDROID_SDK_DIR}/anki/bin/buck
popd
popd


# Write out package info
echo "Pkg.Desc = Android SDK" > ${ANKI_ANDROID_SDK_DIR}/anki/source.properties
echo "Pkg.Revision = ${ANKI_ANDROID_SDK_VERSION}" >> ${ANKI_ANDROID_SDK_DIR}/anki/source.properties

ANKI_ANDROID_SDK_DIST_ZIP=${DIST_DIR}/${ANKI_ANDROID_SDK_NAME}.zip

mkdir ${DIST_DIR}
pushd ${BUILD_DIR}
echo "Making ${ANKI_ANDROID_SDK_DIST_ZIP}...."
zip -r ${ANKI_ANDROID_SDK_DIST_ZIP} ${ANKI_ANDROID_SDK_NAME}
popd


STATARG="-f"
STATFMT="%z"
if [ $HOST_PLATFORM = "linux" ]; then
    STATARG="-c"
    STATFMT="%s"
fi

SHA1SUM=`shasum -a 1 ${ANKI_ANDROID_SDK_DIST_ZIP} | awk '{print $1;}'`
SIZE=`stat $STATARG $STATFMT ${ANKI_ANDROID_SDK_DIST_ZIP}`

cat <<EOF
sdk_info =
{
  "$ANKI_ANDROID_SDK_VERSION" : {"size": $SIZE, "sha1": "$SHA1SUM"},
}
EOF

rm -rf ${ANKI_ANDROID_SDK_DIR}

