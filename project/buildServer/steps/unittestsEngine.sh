#!/usr/bin/env bash
#
set -eu

# change dir to the script dir, no matter where we are executed from
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPTNAME=="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
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

while getopts "hvc:p" opt; do
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
LOG=cozmoEngineGoogleTest.log
LOGZIP=cozmoEngineGoogleTest.tar.gz

CTEST="ctest --output-on-failure -O ${LOG}"

if (( ${VERBOSE} )); then
  CTEST="${CTEST} -V"
fi

if [ ! ${ANKICONFIGROOT:+x} ]; then
  export ANKICONFIGROOT=$BUILDPATH
fi
if [ ! ${ANKIWORKROOT:+x} ]; then
  export ANKIWORKROOT=$BUILDPATH
fi


echo "Copying animation assets for tests"  
${TOPLEVEL}/tools/animationScripts/copy_anims_for_test.py

# unit test(s) look for this and dump a file if it's set
# uncomment to generate a build artifact of all behaviors and their relationships
export ANKI_TEST_BEHAVIOR_FILE="behavior_transitions.txt"
export ANKI_TEST_BEHAVIOR_BRANCHES="behavior_branches.txt"
export ANKI_TEST_BEHAVIOR_FEATURES="behavior_active_features.txt"


# The https://github.com/anki/voice-intent-resolution-config repo is available as a submodule at
# cloud/voice-intent-resolution-config/ and that provides the primary voice intent resolution data
# from the main Dialogflow project. Meanwhile, there is additional voice intent resolution data in
# a 'victor-games-dev-en-us' Dialogflow project, but that data doesn't (yet) live in any git repo,
# so this script has to grab that secondary voice intent resolution data directly from Dialogflow
# and merge it with the primary data.

VOICE_INTENT_DIR="cloud/voice-intent-resolution-config"
VOICE_INTENT_DIR="${TOPLEVEL}/${VOICE_INTENT_DIR}"
VOICE_INTENT_DATA_DIR="${VOICE_INTENT_DIR}/dialogflow-en-us"

OTHER_DIALOGFLOW_PROJECT="victor-games-dev-en-us"

export DIALOGFLOW_PRIVATE_KEY="-----BEGIN PRIVATE KEY-----\nMIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQCwgTrwV7fpSDFU\nWkRsuXN06cTC8D12nGnjRzUsUL8omBZKPE1fw0/P22eGqGzLVfpw1cu1cFPtk7kW\nlSEebnF/sS+LX+gxpgnDyy02MWwreHCC5hjn7c/TLNzHCfj47cvjEN9elo1PowbZ\nNuUa53di/yo8x7SNHGO8B2JOeF9XdQ9hu4D3DHXf7+ua3MufNnqHEUGNowFgMKMl\nIJlEaf+yONLBlGrMfEBfIsi9stvDmnKCx9Tq78N84etD621vsCBfkqxR0g50fr24\nE5Xwd4URUi125b8d8SDQ4Gb7AjP94pQ9zopGKoUZnGM0AhYphslJWy0f7HeXcp30\nr63RA3tdAgMBAAECggEABjvCI5vNdHEmwNZhRT+1aqMY3lOSsB4tBShOIe8GGT5p\nxxxjFSc2Kd6TROes0++I5TQ/ufwqEqFdb4U7wrHe00YkKjVsY1h8Tsxdn+TV76EU\nxCKxLXuyE8CpWWQqRUxMn4vIf7QscGwPAVvVcoJLLpVZXgUlhLhEc28FU0UgBNwf\nz0Fgd9Bx6QTJMwkyl8quaK9HJUaO6FFbXK2Dh/9mFignLehR4dPZ7HdUNgAZ2eJT\nqSCFo9AufJ/m+RmefgThgnJ8ePCPNydQGFIh65vIMdT+gNN5aw7AWv7/o4It8a3d\nRlnUqY4131IIgX7gqPqxgTWepXGCLYm9DMhAeS91oQKBgQD1wdSqgQ3gQxGs7Mmt\nbmsQZNRQHFP18QjbgMOcPjW0+Ht4OHNuV0bKFU+0kTSWS4h78bE/X0IDttylO3nL\nCp6TdnbljC7dES0GLX2JY4SZqk7XTs+OVvgx5wDjFRSICejKLIV157KJDrmTNPWQ\nAZI+8N0H52jGZTl+1XDak4NNbQKBgQC33H5mZRTKpyIF2QBpPRjRO/0kQrh8/G04\nn5DI3yrvsJaMd9GmHN0BAoKl3LYIowV1qMsQuYuJ0Zdf6i7DHoPqOgSc+INgUS0w\na4ki+iambSF+xO4Rhverv2qIivlmuolcw5KFbAg5XucI5oHOO35bynWlBSz2fIFI\nbnZELU3fsQKBgQCqyRDpjNX/y5w0+DkPdXSh5/BD2+vdNqxZHprRscnRAf2MBm7x\nd2WSekzxxxcse0FWIRh0Wdaeji24BiUVnUOmZuUpkMngh1cqu+JJ4Ab+YwR3TCWh\nXX8N7uMG7FdgUsKb/WSiE2pXJQyB6IPel7jyVKDGJWMCvMXABsRuoaTV4QKBgQCf\nOrjJCt9fxjDD6bPecEgu5IoNvi6yJ5abhC1KmWNA46juC9KnzrVja34kEKohfrV6\nuWzrlhUPjVFQgqpu1t2dmxNlsh9s6cB9/5NrlEKmvTpV5EzdJwsVVZf4mor1ebT+\nfm1FWVMiBFuHrMFcDtAWxJbwDDRtX1RDm06XKrkXcQKBgDWyZPbESM4LmDpiGd5I\nG/3y0zTLcnUtKPgqdfbLaKF/nkhW4KytdJuT902shb7FvuUfR5y6N3rmQtTJCjei\nkY7RP23IIcN27UDUzZQMV6un6sYa5LowNjkKpCYB4SzjgCUHGw4tsYVWq8v9L9tV\nfZTMYO4iQ55LgGfneATBvUQi\n-----END PRIVATE KEY-----\n"

# Engine unit test(s) look for this file to run cloud intent tests
export ANKI_TEST_INTENT_SAMPLE_FILE="/tmp/intent_samples_for_test.json"

# Grab the latest master version of the primary voice intent resolution data and tools from git
SUBMODULE_COMMIT_ID=$(git submodule status $VOICE_INTENT_DIR | awk '{print $1}')
pushd ${VOICE_INTENT_DIR}
git checkout master
git pull
popd

# Grab the secondary voice intent resolution data directly from Dialogflow and merge it with the primary data
DIALOGFLOW_PULL_SCRIPT=${VOICE_INTENT_DIR}/lib/util/tools/dialogflow/dialogflow_download.py
TMPDIR=$(mktemp -d)
ZIP_FILE=$(mktemp).zip
python2 $DIALOGFLOW_PULL_SCRIPT --project_id $OTHER_DIALOGFLOW_PROJECT --unpack_dir $TMPDIR --zip_file $ZIP_FILE
cp $TMPDIR/intents/*.json ${VOICE_INTENT_DATA_DIR}/intents/
cp $TMPDIR/entities/*.json ${VOICE_INTENT_DATA_DIR}/entities/
rm -rf $TMPDIR $ZIP_FILE

# Generate the json file of Dialogflow sample intents that will be used for the UserIntentsParsing.CloudSampleFileParses test
rm -f $ANKI_TEST_INTENT_SAMPLE_FILE
${TOPLEVEL}/tools/ai/makeSampleIntents.py $VOICE_INTENT_DATA_DIR $ANKI_TEST_INTENT_SAMPLE_FILE

# Restore the submodule directory for the primary voice intent resolution data back to its original
# state (before we pulled the latest master version and then merged in the secondary data)
pushd ${VOICE_INTENT_DIR}
git reset --hard
git clean -f
git checkout ${SUBMODULE_COMMIT_ID}
popd

# Done with the setup for the file needed to run cloud intent tests (ANKI_TEST_INTENT_SAMPLE_FILE)


echo "Entering directory \`${BUILDPATH}'"
cd ${BUILDPATH}

# clean
rm -rf ${XML} ${LOG} ${LOGZIP}

# prepare
mkdir -p testdata

# execute
set +e
set -o pipefail

${CTEST}

EXIT_STATUS=$?

# additional files generated by tests we want to save, filled in below
FILES="" 

## disabled automatic generation of behaviors pdf file
# if [[ ${ANKI_TEST_BEHAVIOR_FILE:+x} ]]; then
#   BEHAVIOR_FILE="${BUILDPATH}/${ANKI_TEST_BEHAVIOR_FILE}"
#   # unit tests may have generated this file
#   if [ -f $BEHAVIOR_FILE ]; then
#     SCRIPT=${TOPLEVEL}/tools/ai/plotBehaviors.py
#     BEHAVIOR_OUT=behaviors.pdf
#     if ${SCRIPT} -o ${BEHAVIOR_OUT} ${BEHAVIOR_FILE}; then
#       FILES="${FILES} ${BEHAVIOR_OUT}"
#     fi
#   fi
# fi

set -e

#  publish results
tar czf ${LOGZIP} ${LOG} ${XML} ${FILES}

# exit
exit $EXIT_STATUS
