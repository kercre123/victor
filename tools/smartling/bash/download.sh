#!/bin/bash

script_dir="$(dirname "$0")"
source "$script_dir/common.sh"

function usage {
cat << EOF
Usage: ${0##*/} [options] dir
OPTIONS:
    -h Show this message.

    -t (REQUIRED) The type of download. One of the following:
                  (${SUPPORTED_DOWNLOAD_TYPES[@]})

    -u (OPTIONAL) The URI mask used to filter the list of files to download.

    -l (OPTIONAL) The list of project locales to consider. For example, "es-ES ja-JP".

                  Applies to all download types except 'original'.

                  By default all locales will be considered.

    -C (OPTIONAL) Use this flag to download completed files only.

    -A (OPTIONAL) Use this flag to append locales to file names instead of creating sub-folders.

                  Applies to all download types except 'original'.

    -S (OPTIONAL) Use this flag to create sub-folders based on the file URI.

    -O (OPTIONAL) Use this flag to include original strings when no translations are available.

                  Applies to the followoing file types only: gettext, javaProperties, custom xml or json.

                  Applies to all download types except 'original'.

    -W (OPTIONAL) Use this flag to overwrite existing files.

                  By default a number is appended to the name of the file if it exists already.
EOF
}

function download_original {
    local files=("${FILES_TO_DOWNLOAD[@]}")

    for file in "${files[@]}"; do
        echo "Downloading $DOWNLOAD_TYPE file \"$file\"..."

        local file_name=$(strip_file_path "$file")

        local dir="$DOWNLOAD_DIR"
        if [[ "$USE_URI_SUB_FOLDERS" == true ]]; then
            local uri_sub_folder=$(get_file_uri_sub_folder "$file")
            dir="$dir/$uri_sub_folder"
        fi
        mkdir_if_necessary "$dir"

        local file_path="$dir/$file_name"
        if [[ "$OVERWRITE_FILES" == true ]]; then
            if [ -f "$file_path" ]; then rm "$file_path"; fi
        else
            file_path=$(get_safe_file_path "$file_path")
        fi

        download_file "$file" "$file_path"
        local http_code=$(get_array_value_by_key "httpcode" "${SM_HTTP_HEADERS[@]}")
        handle_download_response "$file" "$file_path" ${http_code}
    done

    return 0
}

function download_translated {
    local files=("${FILES_TO_DOWNLOAD[@]}")
    local locales=("${LOCALES[@]}")

    for file in "${files[@]}"; do
        for locale in "${locales[@]}"; do
            echo "Downloading $DOWNLOAD_TYPE file \"$file\" for locale \"$locale\"..."

            local file_name=$(strip_file_path "$file")

            local dir="$DOWNLOAD_DIR"
            if [[ "$APPEND_LOCALES" == false ]]; then
                dir="$dir/$locale"
            else
                file_name="$locale-$file_name"
            fi
            if [[ "$USE_URI_SUB_FOLDERS" == true ]]; then
                local uri_sub_folder=$(get_file_uri_sub_folder "$file")
                dir="$dir/$uri_sub_folder"
            fi
            mkdir_if_necessary "$dir"

            local file_path="$dir/$file_name"
            if [[ "$OVERWRITE_FILES" == true ]]; then
                if [ -f "$file_path" ]; then rm "$file_path"; fi
            else
                file_path=$(get_safe_file_path "$file_path")
            fi

            if [[ "$COMPLETED_ONLY" == true ]]; then
                check_file_status "$file" "$locale"
                # print_file_status_response "$SM_RESPONSE"
                if [[ "$SM_FILE_STATUS" != "completed" ]]; then
                    continue
                fi
            fi

            download_file "$file" "$file_path" "$locale" "$DOWNLOAD_TYPE" "$INCLUDE_ORIGINAL"
            local http_code=$(get_array_value_by_key "httpcode" "${SM_HTTP_HEADERS[@]}")

            # sync file extension if necessary
            # (for some formats translated files may have dfferent extensions than the original ones)
            if [[ ${http_code} -eq 200 ]]; then
                local response_file_name=$(get_array_value_by_key "filename" "${SM_HTTP_HEADERS[@]}")
                file_path=$(set_file_extension_if_different "$file_path" $(get_file_extension "$response_file_name"))
            fi

            handle_download_response "$file" "$file_path" ${http_code}
        done
    done

    return 0
}

function handle_download_response {
    local file="$1"
    local file_path="$2"
    local http_code="$3"

    if [[ ${http_code} -eq 200 ]]; then
        echo "Downloaded \"$file\" to \"$file_path\"."
    else
        local error_message="Error downloading \"$file\". HTTP code: ${http_code}"
        if [[ -f "$file_path" ]]; then
            error_response=`cat "$file_path"`
            print_error_response "$error_message:" "$error_response"
            rm "$file_path"
        else
            echo "$error_message"
        fi
    fi
    echo
}

# get options
DOWNLOAD_TYPE=""
URI_MASK=""
LOCALES=()
COMPLETED_ONLY=false
APPEND_LOCALES=false
USE_URI_SUB_FOLDERS=false
INCLUDE_ORIGINAL=true
OVERWRITE_FILES=false

OPTERR=0
OPTIND=1
while getopts ":ht:u:l:CASOW" opt; do
    case "$opt" in
    h)
        usage && exit 0
        ;;
    t)
        DOWNLOAD_TYPE=$OPTARG
        ;;
    u)
        URI_MASK=$OPTARG
        ;;
    l)
        LOCALES=($OPTARG)
        ;;
    C)
        COMPLETED_ONLY=true
        ;;
    A)
        APPEND_LOCALES=true
        ;;
    S)
        USE_URI_SUB_FOLDERS=true
        ;;
    O)
        INCLUDE_ORIGINAL=$OPTARG
        ;;
    W)
        OVERWRITE_FILES=true
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
if [[ -z "$DOWNLOAD_TYPE" ]]; then
    warn_and_exit
elif ! is_supported_download_type "$DOWNLOAD_TYPE"; then
    warn_and_exit "Unsupported download type: $DOWNLOAD_TYPE"
fi

DOWNLOAD_DIR="$1"
if [[ -z "$DOWNLOAD_DIR" ]]; then
    warn_and_exit "The download directory is required"
else
    mkdir_if_necessary "$DOWNLOAD_DIR"
fi

curl_silent
list_files "$URI_MASK"
exit_code=$?
curl_restore
if [[ ${exit_code} -eq 0 ]]; then
    FILES_TO_DOWNLOAD=("${SM_FILES[@]}")
    if [[ ${#FILES_TO_DOWNLOAD[@]} -eq 0 ]]; then
        echo "No files to download found"
        exit 0
    else
        echo "The number of files matched: ${#FILES_TO_DOWNLOAD[@]}"
    fi
else
    print_response_error "Listing of files failed:" "$SM_RESPONSE"
    exit ${exit_code}
fi

if [[ "$DOWNLOAD_TYPE" != "original" ]]; then
    if [[ "${#LOCALES[@]}" -eq 0 ]]; then
        curl_silent
        list_locales
        exit_code=$?
        curl_restore
        if [[ ${exit_code} -eq 0 ]]; then
            LOCALES=("${SM_LOCALES[@]}")
         else
            print_response_error "Listing of locales failed:" "$SM_RESPONSE"
            exit ${exit_code}
        fi
    fi
    echo "The locales to use: (${LOCALES[@]})"
fi
echo

if [[ "$DOWNLOAD_TYPE" == "original" ]]; then
    download_original
else
    download_translated
fi

