#!/bin/bash

script_dir="$(dirname "$0")"
source "$script_dir/settings.sh"
source "$script_dir/globals.sh"

OLD_CURL_OPTIONS=$CURL_OPTIONS

function curl_silent {
    OLD_CURL_OPTIONS=$CURL_OPTIONS
    CURL_OPTIONS=(--silent)
}

function curl_restore {
    CURL_OPTIONS=$OLD_CURL_OPTIONS
}

function warn_and_exit {
    if [[ -n "$1" ]]; then echo $'\n'"$1"$'\n'; fi
    usage
    exit 1
}

function verify_settings {
    if [[ -z "$API_KEY" || -z "$PROJECT_ID" ]]; then
        echo "Please set your API_KEY and/or PROJECT_ID in settings"
        exit 1
    fi

    if [[ -z "$API_VERSION" ]]; then
        API_VERSION='v1'
    fi

    verify_api_available
}

function get_base_url {
    echo "https://$API_SERVICE_URL/$API_VERSION"
}

function verify_api_available {
    curl_silent
    # test settings and availability by fetching a list of project locales
    list_locales
    local exit_code=$?
    curl_restore
    if [[ ${exit_code} != 0 ]]; then
        print_error_response "Error validating server connectivity:" "$SM_RESPONSE"
        echo "Please verify your API key and Project ID settings."
        exit ${exit_code}
    fi
}

function get_response_code {
    echo $(get_value_using_regex "$1" "$RESPONSE_CODE_REGEX")
}

function get_response_messages {
    echo $(get_value_using_regex "$1" "$RESPONSE_MESSAGES_REGEX")
}

function print_error_response {
    local code="$(get_response_code "$2")"
    local messages="$(get_response_messages "$2")"
    printf "%s [\n\tCode: %s\n\tMessages: [%s]\n]\n" "$1" "$code" "$messages"
}

function get_value_using_regex {
    local str="$1"
    local regex="$2"
    if [[ "$str" =~ $regex ]]; then
        echo "${BASH_REMATCH[1]}"
    else
        echo ""
    fi
}

function string_to_array {
    SM_ARRAY=()
    local str="$1"
    while read -r line; do
        SM_ARRAY+=("$line")
    done <<< "$str"
}

function array_contains_value {
    local el;
    for el in "${@:2}"; do
        [[ "$el" == "$1" ]] && return 0
    done
    return 1
}

function get_array_value_by_key {
    local key="$1"
    shift
    local array=("$@")

    local value=""
    for element in "${array[@]}"; do
        if [[ "$element" == "${key}="* ]] ; then
            value="${element#${key}=}"
            break
        fi
    done
    echo "$value"
}

function unescapeJson {
    echo "$1" | sed -e 's/\\\\/\\/g' -e 's/\\"/"/g'
}

function is_supported_file_type {
    array_contains_value "$1" "${SUPPORTED_FILE_TYPES[@]}"
    return $?
}

function is_supported_file_uri_scheme {
    array_contains_value "$1" "${SUPPORTED_FILE_URI_SCHEMES[@]}"
    return $?
}

function is_supported_download_type {
    array_contains_value "$1" "${SUPPORTED_DOWNLOAD_TYPES[@]}"
    return $?
}

function is_supported_condition {
    array_contains_value "$1" "${SUPPORTED_CONDITIONS[@]}"
    return $?
}

function is_supported_glossary_type {
    array_contains_value "$1" "${SUPPORTED_GLOSSARY_TYPES[@]}"
    return $?
}

function is_supported_tmx_type {
    array_contains_value "$1" "${SUPPORTED_TMX_TYPES[@]}"
    return $?
}

function mkdir_if_necessary {
    local dir="$1"
    if [[ ! -d "$dir" ]]; then
        mkdir -p "$dir"
    fi
}

function strip_file_path {
    local file="$1"
    echo "${file##*/}"
}

function strip_file_extension {
    local file="$1"
    echo "${file%.*}"
}

function get_file_extension {
    local file="$1"
    echo "${file##*.}"
}

function get_file_uri_sub_folder {
    # remove wierd chars from the beginning
    local sub_folder=$(echo "$1" | sed -e 's/^[-~!@#\$%\^&\*\+\.\/\\?:\* ]*//g' )
    echo $(dirname "$sub_folder")
}

function get_safe_file_path {
    local file_path="$1"

    # add a number to the file name if file exists already
    if [[ -e "$file_path" ]] ; then
        local i=0
        local new_file_path="$file_path"
        while [[ -e "$new_file_path" ]] ; do
            let i++;
            new_file_path="${file_path%.*}@$i.${file_path##*.}"
        done
        file_path="$new_file_path"
    fi

    echo "$file_path"
}

function set_file_extension_if_different {
    local file_path="$1"
    local desired_ext="$2"

    local file_ext=$(get_file_extension "$file_path")
    if [[ "$desired_ext" != "$file_ext" ]]; then
        local new_file_path=$(strip_file_extension "$file_path")".$desired_ext"
        mv "$file_path" "$new_file_path"
        echo "$new_file_path"
    else
        echo "$file_path"
    fi
}

function append_timestamp_to_file_name {
    local dir="$1"
    local file_name="$2"
    local now=$(date +"%Y%m%d-%H%M%S")
    local new_file_name="$(strip_file_extension "$file_name")_$now.$(get_file_extension "$file_name")"
    mv "$dir/$file_name" "$dir/$new_file_name"
    echo "$new_file_name"
}

function list_locales {
    SM_LOCALES=()

    local url=$(get_base_url)"/project/locale/list"
    local params=(
        --data-urlencode "apiKey=$API_KEY"
        --data-urlencode "projectId=$PROJECT_ID"
    )

    SM_RESPONSE=$(curl -G $CURL_OPTIONS "${params[@]}" "$url")
    SM_CODE=$(get_response_code "$SM_RESPONSE")
    if [[ "$SM_CODE" != "SUCCESS" ]]; then return 1; fi

    local locales=$(echo "$SM_RESPONSE" | grep -oEi "$LOCALE_REGEX" | cut -c11- | rev | cut -c2- | rev)
    string_to_array "$locales"
    SM_LOCALES+=("${SM_ARRAY[@]}")

    return 0
}

function list_files {
    SM_FILES=()
    local uri_mask="$1"
    shift
    local conditions=("$@")

    local url="$(get_base_url)/file/list"

    local params=(
        --data-urlencode "apiKey=$API_KEY"
        --data-urlencode "projectId=$PROJECT_ID"
    )
    if [[ -n "$uri_mask" ]]; then
        params+=(--data-urlencode "uriMask=$uri_mask")
    fi
    for condition in "${conditions[@]}"; do
        params+=(--data-urlencode "conditions=$condition")
    done

    local files_count=0
    local offset=0
    local batch_size=100

    # list files in batches to overcome File APIs limit
    local count=1
    while [[ ${offset} -eq 0 || ${offset} -lt ${files_count} ]]
    do
        local batch_params=("${params[@]}")
        batch_params+=(--data-urlencode "offset=$offset")
        batch_params+=(--data-urlencode "limit=$batch_size")

        SM_RESPONSE=$(curl -G $CURL_OPTIONS "${batch_params[@]}" "$url")
        SM_CODE=$(get_response_code "$SM_RESPONSE")
        if [[ "$SM_CODE" != "SUCCESS" ]]; then return 1; fi

        if [[ ${files_count} -eq 0 ]]; then
            files_count=$(get_value_using_regex "$SM_RESPONSE" "$FILE_COUNT_REGEX")
            if [[ ${files_count} -eq 0 ]]; then break; fi
        fi

        if [[ $(grep --version) == *"GNU"* ]]; then
            grep_opts='-oPi'
        else
            grep_opts='-oEi'
        fi

        local file_uris=$(echo "$SM_RESPONSE" | grep $grep_opts "$FILE_URI_REGEX" | cut -c12- | rev | cut -c3- | rev)
        string_to_array "$file_uris"
        for file_uri in "${SM_ARRAY[@]}"; do
            file_uri=$(unescapeJson "$file_uri")
            SM_FILES+=("$file_uri")
        done

        offset=$((offset + batch_size))
    done

    return 0
}

function get_file_status {
    local file_uri="$1"
    local locale="$2"

    local url="$(get_base_url)/file/status"

    local params=(
        --data-urlencode "apiKey=$API_KEY"
        --data-urlencode "projectId=$PROJECT_ID"
        --data-urlencode "fileUri=$file_uri"
        --data-urlencode "locale=$locale"
    )

    SM_RESPONSE=$(curl -G $CURL_OPTIONS "${params[@]}" "$url")
    SM_CODE=$(get_response_code "$SM_RESPONSE")
    if [[ "$SM_CODE" != "SUCCESS" ]]; then return 1; fi

    return 0
}

function print_file_status_response {
    local response="$1"
    echo "File status results: ["
    echo $'\t'"Word count: "$(get_value_using_regex "$response" "$WORD_COUNT_REGEX")
    echo $'\t'"String count: "$(get_value_using_regex "$response" "$STRING_COUNT_REGEX")
    echo $'\t'"Approved string count: "$(get_value_using_regex "$response" "$APPROVED_STRING_COUNT_REGEX")
    echo $'\t'"Completed string count: "$(get_value_using_regex "$response" "$COMPLETED_STRING_COUNT_REGEX")
    echo "]"
}

function check_file_status {
    SM_FILE_STATUS=""

    get_file_status "$1" "$2"
    if [[ $? -eq 0 ]]; then
        local string_count=$(get_value_using_regex "$SM_RESPONSE" "$STRING_COUNT_REGEX")
        local completed_count=$(get_value_using_regex "$SM_RESPONSE" "$COMPLETED_STRING_COUNT_REGEX")

        if [[ ${string_count} -gt 0 && ${string_count} -eq ${completed_count} ]]; then
            SM_FILE_STATUS="completed"
        fi

        return 0
     fi

     return 1
}

function delte_file {
    local file_uri="$1"

    local url=$(get_base_url)"/file/delete"

    local params=(
        --data-urlencode "apiKey=$API_KEY"
        --data-urlencode "projectId=$PROJECT_ID"
        --data-urlencode "fileUri=$file_uri"
    )

    SM_RESPONSE=$(curl -X DELETE $CURL_OPTIONS "${params[@]}" "$url")
    SM_CODE=$(get_response_code "$SM_RESPONSE")
    if [[ "$SM_CODE" != "SUCCESS" ]]; then return 1; fi

    return 0
}

function print_delete_response {
    local response="$1"
    echo "File deleted."
}

function upload_file {
    local file="$1"
    local file_uri="$2"
    local file_type="$3"
    local custom_placeholder="$4"
    local callback_url="$5"

    local url="$(get_base_url)/file/upload"

    local params=(
        -F "apiKey=$API_KEY"
        -F "projectId=$PROJECT_ID"
        -F "file=@$file"
        -F "fileUri=$file_uri"
        -F "fileType=$file_type"
    )
    if [[ -n "$custom_placeholder" ]]; then
        params+=(-F "smartling.placeholder_format_custom=$custom_placeholder")
    fi
    if [[ -n "$callback_url" ]]; then
        params+=(-F "callbackUrl=$callback_url")
    fi

    SM_RESPONSE=$(curl $CURL_OPTIONS "${params[@]}" "$url")
    SM_CODE=$(get_response_code "$SM_RESPONSE")
    if [[ "$SM_CODE" != "SUCCESS" ]]; then return 1; fi

    return 0
}

function print_upload_response {
    local response="$1"
    echo "Upload results: ["
    echo $'\t'"Word count: "$(get_value_using_regex "$response" "$WORD_COUNT_REGEX")
    echo $'\t'"String count: "$(get_value_using_regex "$response" "$STRING_COUNT_REGEX")
    echo $'\t'"Overwitten: "$(get_value_using_regex "$response" "$OVERWRITTEN_REGEX")
    echo "]"
}

function download_file {
    local file_uri="$1"
    local file_path="$2"
    local locale="$3"
    local retrieval_type="$4"
    local include_original="$5"

    local url="$(get_base_url)/file/get"

    local params=(
        --data-urlencode "apiKey=$API_KEY"
        --data-urlencode "projectId=$PROJECT_ID"
        --data-urlencode "fileUri=$file_uri"
    )
    if [[ -n "$locale" ]]; then
        params+=(--data-urlencode "locale=$locale")
    fi
    if [[ -n "$retrieval_type" ]]; then
        params+=(--data-urlencode "retrievalType=$retrieval_type")
    fi
    if [[ -n "$include_original" ]]; then
        params+=(--data-urlencode "includeOriginalStrings=$include_original")
    fi

    local headers_file_path=$(get_http_headers_file_path "$file_path")
    local http_code=$(curl -w "%{http_code}" -G $CURL_OPTIONS "${params[@]}" "$url" -D "$headers_file_path" -o "$file_path")

    process_http_headers "$headers_file_path"
    SM_HTTP_HEADERS+=("httpcode=${http_code}")

    if [[ ${http_code} -eq 200 ]]; then return 0; fi
    return 1
}

function get_http_headers_file_path {
    local file_path="$1"
    local file_dir=$(dirname "$file_path")
    local file_name=$(basename "$file_path")
    local file_name_without_ext=$(strip_file_extension "$file_name")

    echo "${file_dir}/${file_name_without_ext}_http_headers.txt"
}

function process_http_headers {
    SM_HTTP_HEADERS=()
    local headers_file_path="$1"
    local headers=`cat "$headers_file_path"`

    local filename=$(get_value_using_regex "$headers" "filename=\"(.+)\";")
    SM_HTTP_HEADERS+=("filename=$filename")

    if [[ "$KEEP_HTTP_HEADERS" != true ]]; then
        rm "$headers_file_path"
    fi
}

function download_glossary {
    local download_dir="$1";shift
    local glossary_type="$1";shift
    local locales=("$@")

    local url="$(get_base_url)/glossary/download"

    local params=(
        --data-urlencode "apiKey=$API_KEY"
        --data-urlencode "projectId=$PROJECT_ID"
        --data-urlencode "exportType=$glossary_type"
    )
    for locale in "${locales[@]}"; do
        params+=(--data-urlencode "locales=$locale")
    done

    local current_dir="$(pwd)"
    cd "$download_dir"
    local curl_response=$(curl -w "%{http_code}\n%{filename_effective}" -G $CURL_OPTIONS -O -J -s "${params[@]}" "$url")
    cd "$current_dir"

    string_to_array "$curl_response"

    local http_code=${SM_ARRAY[0]}
    if [[ ${http_code} -eq 200 ]]; then
        local file_name=${SM_ARRAY[1]}
        local new_file_name=$(append_timestamp_to_file_name "$download_dir" "$file_name")
        echo "Downloaded glossary file \"$new_file_name\" to \"$download_dir\""
        return 0
    else
        echo "Expected HTTP code 200 but recieved $http_code instead. Try running curl in verbose mode to see possible issues."
        return 1
    fi
}

function download_tmx {
    local download_dir="$1"
    local tmx_type="$2"
    local locale="$3"

    local url="$(get_base_url)/translations/download"

    local params=(
        --data-urlencode "apiKey=$API_KEY"
        --data-urlencode "projectId=$PROJECT_ID"
        --data-urlencode "dataSet=$tmx_type"
        --data-urlencode "locale=$locale"
    )

    local current_dir="$(pwd)"
    cd "$download_dir"
    local curl_response=$(curl -w "%{http_code}\n%{filename_effective}" -G $CURL_OPTIONS -O -J -s "${params[@]}" "$url")
    cd "$current_dir"

    string_to_array "$curl_response"

    local http_code=${SM_ARRAY[0]}
    if [[ ${http_code} -eq 200 ]]; then
        local file_name=${SM_ARRAY[1]}
        local new_file_name=$(append_timestamp_to_file_name "$download_dir" "$file_name")
        echo "Downloaded TMX file \"$new_file_name\" to \"$download_dir\""
        return 0
    else
        echo "Expected HTTP code 200 but recieved $http_code instead. Try running curl in verbose mode to see possible issues."
        return 1
    fi
}

verify_settings
