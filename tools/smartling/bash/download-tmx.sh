#!/bin/bash

script_dir="$(dirname "$0")"
source "$script_dir/common.sh"

function usage {
cat << EOF
Usage: ${0##*/} [options] dir
OPTIONS:
    -h Show this message.

    -t (OPTIONAL) The type of TMX download. One of the following: (${SUPPORTED_TMX_TYPES[@]}).

                  Default is '${SUPPORTED_TMX_TYPES[0]}'.

    -l (OPTIONAL) The list of project locales to consider. For example, "es-ES ja-JP".

                  By default all locales will be considered.
EOF
}

# get options
TMX_TYPE="${SUPPORTED_TMX_TYPES[0]}"
LOCALES=()

OPTERR=0
OPTIND=1
while getopts ":ht:l:" opt; do
    case "$opt" in
    h)
        usage && exit 0
        ;;
    t)
        TMX_TYPE=$OPTARG
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
if [[ -n "$TMX_TYPE" ]]; then
    if ! is_supported_tmx_type "$TMX_TYPE"; then
        warn_and_exit "Unsupported TMX type: $TMX_TYPE"
    fi
fi

DOWNLOAD_DIR="$1"
if [[ -z "$DOWNLOAD_DIR" ]]; then
    warn_and_exit "The download directory is required"
else
    mkdir_if_necessary "$DOWNLOAD_DIR"
fi

curl_silent
if [[ "${#LOCALES[@]}" -eq 0 ]]; then
    list_locales
    if [[ $? -eq 0 ]]; then
        LOCALES=("${SM_LOCALES[@]}")
     else
        print_response_error "Listing of locales failed:" "$SM_RESPONSE"
        exit 1
    fi
fi
echo "The locales to use: (${LOCALES[@]})"
curl_restore

for locale in "${LOCALES[@]}"; do
    download_tmx "$DOWNLOAD_DIR" "$TMX_TYPE" "$locale"
done
