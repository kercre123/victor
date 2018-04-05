#!/bin/bash
#
# Author: chapados
# Date: 02/16/2018
# Copyright 2018 Anki, Inc.
#
# Takes a manifest listing of files relative to a target dir,
# compares them to the contents of target dir and deletes
# files not in the manifest.
#
# Outputs the list of deleted files
#

set -e
set -u
set -o pipefail

usage()
{
    echo "Usage: $(basename $0) [ -c ] [ -o output_file ] -m manifest_file <target dir>"
    echo "Remove files in target directory that are not found in manifest file."
    echo ""
    echo "-h                        Print this message"
    echo "-v                        Verbose log output"
    echo "-c                        Manifest is in CMake format (';' == '\n')"
    echo "-m MANIFEST_FILE          List of files in target directory"
    echo "-o OUTPUT_FILE            Write output to file"
    echo ""
}

logv()
{
    if [ $VERBOSE -eq 1 ]; then echo "$1"; fi
}

VERBOSE=0
CMAKE=0

while getopts ":hvcm:o:" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        v)
            VERBOSE=1
            ;;
        o)
            OUTPUT_FILE=${OPTARG}
            ;;
        m)
            MANIFEST_FILE=${OPTARG}
            ;;
        c)
            CMAKE=1
            ;;
        :)
            echo "Option -${OPTARG} required an argument." >&2
            usage
            exit 1
            ;;
    esac
done

shift $(expr $OPTIND - 1)

# manifest is required
if [ -z $MANIFEST_FILE ]; then
    usage
    exit 1
fi

# target directory is required
if [ $# -ne 1 ]; then
    usage
    exit 1;
fi

# default to STDOUT if -o is not specified
OUTPUT=${OUTPUT_FILE:-/dev/stdout}

# This is the base dir for temporarily generated files
BUILD_TMP_DIR=$(dirname ${MANIFEST_FILE})

TARGET_DIR="$1"
logv "TARGET_DIR is ${TARGET_DIR}"

TMP_MANIFEST_LIST="${BUILD_TMP_DIR}/_gc_sorted_manifest.$$.lst"
TMP_DIR_LIST="${BUILD_TMP_DIR}/_gc_sorted_dir.$$.lst"
TMP_GC_LIST="${BUILD_TMP_DIR}/_gc_assets_prune.$$.lst"

# read manifest list and sort files lexographically
if [ $CMAKE -eq 1 ]; then
    logv "Manifest is generated from CMake"
    tr ';' '\n' < ${MANIFEST_FILE} | sort > ${TMP_MANIFEST_LIST}
else
    sort ${MANIFEST_FILE} > ${TMP_MANIFEST_LIST}
fi
logv "read $(wc -l ${TMP_MANIFEST_LIST}) files from manifest"

pushd ${TARGET_DIR} > /dev/null 2>&1

# find all files in build dir & sort lexographically
find . -type f -print | sed 's|^\./||' | sort > ${TMP_DIR_LIST}
logv "found $(wc -l ${TMP_DIR_LIST}) existing files in ${TARGET_DIR}"

# compare sorted lists, printing files that only exist in $TMP_DIR_LIST
comm -13 ${TMP_MANIFEST_LIST} ${TMP_DIR_LIST} > ${TMP_GC_LIST}

# garbage collect stale assets
date > ${OUTPUT}
echo "--- deleted stale files from ${TARGET_DIR} ---" >> ${OUTPUT}
cat ${TMP_GC_LIST} >> ${OUTPUT}

cat ${TMP_GC_LIST} | xargs rm -f

logv "rm $(wc -l ${TMP_GC_LIST}) stale files"

# remove tmp files
rm -f ${BUILD_TMP_DIR}/_gc_*.lst

popd > /dev/null 2>&1
