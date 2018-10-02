#!/usr/bin/env bash

set -e

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=`basename ${0}`
INSPECT=0

function usage() {
    echo "$SCRIPT_NAME [OPTIONS]"
    echo "  -h                  print this message"
    echo "  -v                  verbose output"
    echo "  -s [SERIAL_NUMBER]  Serial number of the robot whose cert to download (ANKI_ROBOT_SERIAL)"
    echo "  -o [OUTPUT_FILE]    Where to download the file (default: /tmp/<esn>.cert)"
    echo "  -i                  Inspect the certificate after downloading"
}

while getopts ":s:o:hiv" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        v)
            set -x
            ;;
        s)
            ANKI_ROBOT_SERIAL="${OPTARG}"
            ;;
        o)
            OUTPUT_FILE="${OPTARG}"
            ;;
        i)
            INSPECT=1
            ;;
        :)
            echo "Option -${OPTARG} required an argument." >&2
            usage
            exit 1
            ;;
    esac
done

if [ -z "${ANKI_ROBOT_SERIAL}" ]; then
    echo "ERROR: must provide serial number of robot to download"
    usage
    exit 1
fi

if [ -z "${OUTPUT_FILE}" ]; then
    OUTPUT_FILE="/tmp/${ANKI_ROBOT_SERIAL}.cert"
fi

echo "> Downloading file to ${OUTPUT_FILE}"
curl https://session-certs.token.global.anki-services.com/vic/${ANKI_ROBOT_SERIAL} > ${OUTPUT_FILE}
echo "> File downloaded: ${OUTPUT_FILE}"

if grep -xq "\-----BEGIN CERTIFICATE-----" ${OUTPUT_FILE}; then 
    echo "> Download successful! ${OUTPUT_FILE}"
else 
    echo "> Download failed!"
    echo
    cat ${OUTPUT_FILE}
    echo
    echo "> Verify that your robot has been logged in properly, and the certificate is in the cloud"
    exit 1
fi

if [ ${INSPECT} -eq 1 ] ; then
    set -x
    openssl x509 -in ${OUTPUT_FILE} -text -noout
fi
