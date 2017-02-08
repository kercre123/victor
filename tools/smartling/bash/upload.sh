#!/bin/bash

script_dir="$(dirname "$0")"
source "$script_dir/common.sh"

function usage {
cat << EOF
Usage: ${0##*/} [options] file ...
OPTIONS:
    -h Show this message.

    -t (REQUIRED) Unique Smartling file type identifier. One of the following:
                  (${SUPPORTED_FILE_TYPES[@]})

    -u (OPTIONAL) File URI scheme. One of the following:
                  (${SUPPORTED_FILE_URI_SCHEMES[@]})

                  Default is '$DEFAULT_FILE_URI_SCHEME'.

    -x (OPTIONAL) The prefix to use in front of the file name for Smartling file URI.

                  Required if specifying URI scheme 'prefix'.

    -p (OPTIONAL) Valid Smartling custom placeholder format.

    -c (OPTIONAL) Callback URL that will be invoked using GET request when a file is 100% published for a locale.

                  The following parameters will be included in the request: fileUri, locale

    -e (OPTIONAL) Set this flag to run in 'test' mode and display the list of files that would have been uploaded.
EOF
}

function upload_files {
    local files=("$@")
    local files_processed=0

    for file in "${files[@]}"; do
        if [[ -f "$file" ]]; then
            local file_uri=$(get_file_uri "$file")
            if [[ "${TEST_MODE}" == true ]]; then
                print_file_info "$file" "$file_uri"
            else
                echo "Uploading [file: \"$file\", uri: \"$file_uri\"]..."
                upload_file "$file" "$file_uri" "$FILE_TYPE" "$CUSTOM_PLACEHOLDER" "$CALLBACK_URL"
                if [[ $? -eq 0 ]]; then
                    print_upload_response "$SM_RESPONSE"
                else
                    print_error_response "Upload failed:" "$SM_RESPONSE"
                fi
            fi
            echo
            ((files_processed++))
        fi
    done

    if [[ ${files_processed} -eq 0 ]]; then
        echo "No files were processed"
    else
        echo "The total number of files processed: $files_processed"
    fi

    return 0
}

function get_file_uri {
    local file="$1"
    if [[ "$FILE_URI_SCHEME" == "relativePath" ]]; then
        echo "$file"
    else
        local file_name=$(basename "$file")
        if [[ "$FILE_URI_SCHEME" == "prefix" ]]; then
            echo "$FILE_URI_PREFIX$file_name"
        else
            echo "$file_name"
        fi
    fi
}

function print_file_info {
    echo "File info: ["
    echo $'\t'"Source: "$1
    echo $'\t'"URI: "$2
    echo $'\t'"Type: $FILE_TYPE"
    echo $'\t'"Custom placeholder: $CUSTOM_PLACEHOLDER"
    echo "]"
}

# get options
FILE_TYPE=""
FILE_URI_SCHEME="$DEFAULT_FILE_URI_SCHEME"
FILE_URI_PREFIX=""
CUSTOM_PLACEHOLDER=""
CALLBACK_URL=""
TEST_MODE=false

OPTERR=0
OPTIND=1
while getopts ":ht:u:x:p:c:e" opt; do
    case "$opt" in
    h)
        usage && exit 0
        ;;
    t)
        FILE_TYPE=$OPTARG
        ;;
    u)
        FILE_URI_SCHEME=$OPTARG
        ;;
    x)
        FILE_URI_PREFIX=$OPTARG
        ;;
    p)
        CUSTOM_PLACEHOLDER=$OPTARG
        ;;
    c)
        CALLBACK_URL=$OPTARG
        ;;
    e)
        TEST_MODE=true
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
if [[ -z "$FILE_TYPE" ]]; then
    warn_and_exit
elif ! is_supported_file_type "$FILE_TYPE"; then
    warn_and_exit "Unsupported file type: $FILE_TYPE"
elif [[ -n "$FILE_URI_SCHEME" ]]; then
    if ! is_supported_file_uri_scheme "$FILE_URI_SCHEME"; then
        warn_and_exit "Unsupported URI scheme: $FILE_URI_SCHEME"
    elif [[ "$FILE_URI_SCHEME" == 'prefix' && -z "$FILE_URI_PREFIX" ]]; then
        warn_and_exit "The prefix is required when '$FILE_URI_SCHEME' file URI scheme is used"
    fi
fi

if [[ "${TEST_MODE}" == true ]]; then
    echo "Running in test mode... No files will be uploaded."
fi

upload_files "$@"
