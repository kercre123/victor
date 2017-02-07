#!/bin/bash

script_dir="$(dirname "$0")"
source "$script_dir/common.sh"

function usage {
cat << EOF
Usage: ${0##*/} [options]
OPTIONS:
    -h Show this message.

    -u (REQUIRED) The URI mask used as a filter to create the list of files to delete.

    -c (OPTIONAL) The list of conditions to apply. Any of the following:
                  (${SUPPORTED_CONDITIONS[@]})
EOF
}

function delete_files {
    local files=("${FILES_TO_DELETE[@]}")

    if [[ $MAX_FILES_TO_DISPLAY -ge ${#files[@]} ]]; then
        echo "Are you sure you want to delete the following files? (Enter 'YES' to continue)";echo
        for i in "${!files[@]}"; do
            echo "$(( i + 1 )). ${files[i]}"
        done
        echo
    else
        echo "Are you sure you want to delete all ${#files[@]} files? (Enter 'YES' to continue)"
    fi

    read answer
    if [[ "$answer" != "YES" ]]; then
        echo;echo "Exiting..."
        return 0
    fi
    echo

    for file in "${files[@]}"; do
        echo "Deleting file \"$file\"..."
        delte_file "$file"
        if [[ $? -eq 0 ]]; then
            print_delete_response "$SM_RESPONSE"
        else
            print_error_response "Deletion failed:" "$SM_RESPONSE"
        fi
        echo
    done

    return 0
}

# get options
URI_MASK=""
CONDITIONS=()

OPTERR=0
OPTIND=1
while getopts ":hu:c:" opt; do
    case "$opt" in
    h)
        usage && exit 0
        ;;
    u)
        URI_MASK=$OPTARG
        ;;
    c)
        CONDITIONS=($OPTARG)
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
if [[ -z "$URI_MASK" ]]; then
    warn_and_exit "The URI mask is required"
fi
if [[ "${#CONDITIONS[@]}" -gt 0 ]]; then
    for condition in "${CONDITIONS[@]}"; do
        if ! is_supported_condition "$condition"; then
            warn_and_exit "Unsupported condition: $condition"
        fi
    done
fi

curl_silent
list_files "$URI_MASK" "${CONDITIONS[@]}"
exit_code=$?
curl_restore
if [[ ${exit_code} -eq 0 ]]; then
    FILES_TO_DELETE=("${SM_FILES[@]}")
    if [[ ${#FILES_TO_DELETE[@]} -eq 0 ]]; then
        echo "No files to delete found"
        exit 0
    else
        echo "The number of files to delete: ${#FILES_TO_DELETE[@]}"
    fi
else
    print_error_response "Listing of files failed:" "$SM_RESPONSE"
    exit ${exit_code}
fi

delete_files
