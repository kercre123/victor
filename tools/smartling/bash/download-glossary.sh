#!/bin/bash

script_dir="$(dirname "$0")"
source "$script_dir/common.sh"

function usage {
cat << EOF
Usage: ${0##*/} [options] dir
OPTIONS:
    -h Show this message.

    -t (OPTIONAL) The type of glossary download. One of the following: (${SUPPORTED_GLOSSARY_TYPES[@]}).

                  Default is '${SUPPORTED_GLOSSARY_TYPES[0]}'.

    -l (OPTIONAL) The list of project locales to consider. For example, "es-ES ja-JP".

                  By default all locales will be considered.
EOF
}

# get options
GLOSSARY_TYPE="${SUPPORTED_GLOSSARY_TYPES[0]}"
LOCALES=()

OPTERR=0
OPTIND=1
while getopts ":ht:l:" opt; do
    case "$opt" in
    h)
        usage && exit 0
        ;;
    t)
        GLOSSARY_TYPE=$OPTARG
        ;;
    l)
        LOCALES=($OPTARG)
        ;;
    \?)
        warn_and_exit "Invalid option: -$OPTARG"
        ;;
    :)
        warn_and_exit "Option -$OPTARG requires an argument"
       ;;
    esac
done
shift $((OPTIND - 1))

# validate options
if [[ -n "$GLOSSARY_TYPE" ]]; then
    if ! is_supported_glossary_type "$GLOSSARY_TYPE"; then
        warn_and_exit "Unsupported glossary type: $GLOSSARY_TYPE"
    fi
fi

DOWNLOAD_DIR="$1"
if [[ -z "$DOWNLOAD_DIR" ]]; then
    warn_and_exit "The download directory is required"
else
    mkdir_if_necessary "$DOWNLOAD_DIR"
fi

download_glossary "$DOWNLOAD_DIR" "$GLOSSARY_TYPE" "${LOCALES[@]}"
