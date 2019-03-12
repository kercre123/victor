#!/usr/bin/env bash

# $GTEST_FILTER is set to "UserIntentsParsing.CloudSampleFileParses" below so
# that is the only unit test that gets executed.

set -eu

# change dir to the script dir, no matter where we are executed from
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPTNAME=="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

GIT=`which git`
if [ -z $GIT ];then
  echo "git not found"
  exit 1
fi

# configure

function usage() {
    echo "$SCRIPTNAME [OPTIONS]"
    echo "  -h                      print this message"
    echo "  -v                      verbose output"
    echo "  -c [CONFIGURATION]      build configuration {Debug,Release}"
    echo "  -p [PLATFORM]           build target platform {android,mac}"
}

: ${PLATFORM:=mac}
: ${CONFIGURATION:=Debug}
: ${VERBOSE:=0}

while getopts "hvc:p:" opt; do
  case $opt in
    h)
      usage
      exit 1
      ;;
    v)
      VERBOSE=1
      ;;
    c)
      CONFIGURATION="${OPTARG}"
      ;;
    p)
      PLATFORM="${OPTARG}"
      ;;
    *)
      usage
      exit 1
      ;;
  esac
done

echo "Entering directory \`${SCRIPTDIR}'"
cd $SCRIPTDIR

: ${TOPLEVEL:=`$GIT rev-parse --show-toplevel`}
BUILDPATH=${TOPLEVEL}/_build/${PLATFORM}/${CONFIGURATION}/test/engine

XML="*GoogleTest.xml"
LOG=cloudVoiceDataGoogleTest.log
LOGZIP=cloudVoiceDataGoogleTest.tar.gz

CTEST="ctest --output-on-failure -O ${LOG}"
if (( ${VERBOSE} )); then
  CTEST="${CTEST} -V"
fi

# The $VOICE_INTENT_DIR environment variable should point at a directory that contains a version of
# the data that gets stored in the https://github.com/anki/voice-intent-resolution-config repo and
# provides the primary voice intent resolution data from the main Dialogflow project. Meanwhile,
# there is additional voice intent resolution data in a 'victor-games-dev-en-us' Dialogflow project,
# but that data doesn't (yet) live in any git repo, so this script has to grab that secondary voice
# intent resolution data directly from Dialogflow and merge it with the primary data.

DATA_DIR="dialogflow-en-us"

# The $VOICE_INTENT_DIR env var should be set already
if [[ ! ${VOICE_INTENT_DIR:+x} ]]; then
  echo "VOICE_INTENT_DIR env var should point at primary voice intent resolution data to be tested"
  exit 1
fi

# If $VOICE_INTENT_DIR points at a relative path, then assume it is relative to the root of Git repo
if [[ ! "$VOICE_INTENT_DIR" = /* ]]; then
  VOICE_INTENT_DIR="${TOPLEVEL}/${VOICE_INTENT_DIR}"
fi

# Abort if $VOICE_INTENT_DIR points at a non-existent directory
if [ ! -d "$VOICE_INTENT_DIR" ]; then
  echo "The ${VOICE_INTENT_DIR} directory is unavailable"
  exit 1
fi

VOICE_INTENT_DATA_DIR="${VOICE_INTENT_DIR}/${DATA_DIR}"

if [ ! -d "$VOICE_INTENT_DATA_DIR" ]; then
  echo "The ${VOICE_INTENT_DATA_DIR} directory is unavailable"
  exit 1
fi
echo "Source of primary voice intent resolution data: ${VOICE_INTENT_DATA_DIR}"

# Backup the directory for the primary voice intent resolution data before merging in the secondary data
PRIMARY_BACKUP_DIR=$(mktemp -d)
cp -pR $VOICE_INTENT_DATA_DIR $PRIMARY_BACKUP_DIR/

OTHER_DIALOGFLOW_PROJECT="victor-games-dev-en-us"
# The $DIALOGFLOW_PRIVATE_KEY environment variable should be set to a private key for ^ project

set +e

# Grab the secondary voice intent resolution data directly from Dialogflow and merge it with the primary data
DIALOGFLOW_PULL_SCRIPT=${VOICE_INTENT_DIR}/lib/util/tools/dialogflow/dialogflow_download.py
SECONDARY_DATA_DIR=$(mktemp -d)
ZIP_FILE=$(mktemp).zip
python2 $DIALOGFLOW_PULL_SCRIPT --project_id $OTHER_DIALOGFLOW_PROJECT --unpack_dir $SECONDARY_DATA_DIR --zip_file $ZIP_FILE
cp ${SECONDARY_DATA_DIR}/intents/*.json ${VOICE_INTENT_DATA_DIR}/intents/
cp ${SECONDARY_DATA_DIR}/entities/*.json ${VOICE_INTENT_DATA_DIR}/entities/
rm -rf $SECONDARY_DATA_DIR $ZIP_FILE

# The UserIntentsParsing.CloudSampleFileParses engine unit test looks for this file to run cloud intent tests
export ANKI_TEST_INTENT_SAMPLE_FILE="${TMPDIR}/intent_samples_for_test.json"
echo "Intent sample file: ${ANKI_TEST_INTENT_SAMPLE_FILE}"

# Generate the json file of Dialogflow sample intents that will be used for the UserIntentsParsing.CloudSampleFileParses test
rm -f $ANKI_TEST_INTENT_SAMPLE_FILE
${TOPLEVEL}/tools/ai/makeSampleIntents.py $VOICE_INTENT_DATA_DIR $ANKI_TEST_INTENT_SAMPLE_FILE
EXIT_STATUS=$?

set -e

# Restore the directory for the primary voice intent resolution data back to its original
# state before the secondary data was merged in
rm -rf $VOICE_INTENT_DATA_DIR
mv "${PRIMARY_BACKUP_DIR}/${DATA_DIR}" $VOICE_INTENT_DIR/
rmdir $PRIMARY_BACKUP_DIR

if [ $EXIT_STATUS -ne 0 ]; then
  echo "Failed to generate file of sample intents for the UserIntentsParsing.CloudSampleFileParses test"
  exit $EXIT_STATUS
fi

# Done with the setup for the file needed to run cloud intent tests ($ANKI_TEST_INTENT_SAMPLE_FILE)


echo "Entering directory \`${BUILDPATH}'"
cd ${BUILDPATH}

# clean
rm -rf ${XML} ${LOG} ${LOGZIP}

# execute
set +e
set -o pipefail

export GTEST_FILTER="UserIntentsParsing.CloudSampleFileParses"
${CTEST}
EXIT_STATUS=$?
unset GTEST_FILTER

# additional files generated by tests we want to save, filled in below
FILES="" 

set -e

# publish results
tar czf ${LOGZIP} ${LOG} ${XML} ${FILES}

exit $EXIT_STATUS

