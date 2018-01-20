#!/bin/bash
#
# Download translated strings files from smartling
#
set -e

_SCRIPT_PATH=$(cd $(dirname ${BASH_SOURCE[0]}) && pwd)                                              
_SCRIPT_NAME=`basename "$0"`
: ${SMARTLING_HOME="${_SCRIPT_PATH}/bash"}

function usage() {
    echo "$_SCRIPT_NAME [-n] [-v] [-C] [-t TRANSLATION_TYPE] [-o OUTPUT_DIR] /path/to/localized-strings"
    echo "  -n            : dry run. Do not perform real commands"
    echo "  -v            : verbose. Print verbose output"
    echo "  -C            : complete. Only download files with 'completed' translations"
    echo "  -t type       : { pending, pseudo, published }"
    echo "                    pending: Any translation that exists for a string will be downloaded"
    echo "                    pseudo:  Download original string with extra characters"
    echo "                    published:  Download only completed translations"
    echo "  -o dir        : output directory where translation directories should be created"
}

set -u

DRYRUN=0
SMARTLING_TRANSLATION_TYPE="pending"
COMPLETED=0
: ${VERBOSE:=1}

function logv()
{
    if [[ $VERBOSE -ne 0 ]]; then
        echo "$@"
    fi
}

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

: ${ANKI_BUILD_BASE_LANG:="en-US"}
: ${ANKI_BUILD_TRANSLATED_LANGS:="de-DE fr-FR ja-JP"}

while getopts ":nvCo:t:" opt; do
    case $opt in
        n)
            DRYRUN=1
            ;;
        v)
            VERBOSE=1
            ;;
        C)
            COMPLETED=1
            ;;
        t)
            SMARTLING_TRANSLATION_TYPE="$OPTARG"
            ;;
        o)
            OUTPUT_DIR="$OPTARG"
            ;;
        :)
            echo "Option -$OPTARG requires an argument." >&2
            usage
            exit 1
            ;;
    esac
done

# check for remaining args
shift $(expr $OPTIND - 1)

if [ $# -ne 1 ]; then
    usage
    exit 1
fi

LOCALIZATION_DIR="$1"

: ${OUTPUT_DIR:="${LOCALIZATION_DIR}"}

TRANS_TYPE=`echo $SMARTLING_TRANSLATION_TYPE | tr "[:upper:]" "[:lower:]"`

SMARTLING_OPTS="-W -S"
if [ $COMPLETED -ne 0 ]; then
    SMARTLING_OPTS="${SMARTLING_OPTS} -C"
fi

function smartling_download()
{
    # Download smartling translations
    ${SMARTLING_HOME}/download.sh \
        -t ${TRANS_TYPE} \
        ${SMARTLING_OPTS} -l "${ANKI_BUILD_TRANSLATED_LANGS}" \
        -u "$1" \
        "${OUTPUT_DIR}"
}

# Use base file names to determine which translated files to download
BASE_FILES=(`ls ${LOCALIZATION_DIR}/${ANKI_BUILD_BASE_LANG}/*.json`)
for base_file in "${BASE_FILES[@]}"; do
    logv "Download translations for string file: $base_file"
    bn=`basename $base_file _base.json`
    smartling_download $bn
done

# Fix newlines & other escape codes
for lang in ${ANKI_BUILD_TRANSLATED_LANGS}; do
    lang_dir="${LOCALIZATION_DIR}/${lang}"
    strings_files=(`ls ${lang_dir}/*.json`)
    for string_file in "${strings_files[@]}"; do
        echo "Post process ${string_file}..."
        # fix newlines
        perl -p -i -e 's/\\n/\\u000a/g' "${string_file}"
        perl -p -i -e 's/\\u23CE/\\u000a/gi' "${string_file}"
        # replace "&quot;" with an escaped double-quote
        perl -p -i -e 's/\&quot;/\\"/g' "${string_file}"
        # replace "&lt;" with "<"
        perl -p -i -e 's/\&lt;/</g' "${string_file}"
        # replace "&gt;" with ">"
        perl -p -i -e 's/\&gt;/>/g' "${string_file}"
    done
done
