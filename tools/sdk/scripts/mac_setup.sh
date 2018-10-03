#!/usr/bin/env bash

set -e

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=`basename ${0}`

function usage() {
    echo "$SCRIPT_NAME [OPTIONS]"
    echo "  -h                         print this message"
    echo "  -v                         verbose output"
    echo "  -c [CERTIFICATE_FILE]      Location of the created cert file"
    echo "  -k [KEY_FILE]              Location of the created key file"
    echo "  -H [CERTIFICATE_HOSTNAME]  Hostname to embed in cert"
    echo "  -C [CERTIFICATE_CONF]      Cert configuration file"
}

#
# defaults
#
CERT_FILE="/tmp/anki/gateway/trust.cert"
KEY_FILE="/tmp/anki/gateway/trust.key"
MAC_VECTOR_HOSTNAME=Vector-Local
CONF_FILE="${SCRIPT_PATH}/mac_cert.conf"

while getopts ":c:k:H:C:hv" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        v)
            set -x
            ;;
        c)
            CERT_FILE="${OPTARG}"
            ;;
        k)
            KEY_FILE="${OPTARG}"
            ;;
        H)
            MAC_VECTOR_HOSTNAME="${OPTARG}"
            ;;
        C)
            CONF_FILE="${OPTARG}"
            ;;
        :)
            echo "Option -${OPTARG} required an argument." >&2
            usage
            exit 1
            ;;
    esac
done

echo "> Making output directories"
mkdir -p $(dirname ${CERT_FILE})
mkdir -p $(dirname ${KEY_FILE})

echo "> Creating key/cert pair"
openssl req -config ${CONF_FILE} \
    -subj "/C=US/ST=California/L=SF/O=Anki/CN=${MAC_VECTOR_HOSTNAME}" \
    -new -x509 -days 1000 -newkey rsa:2048 -nodes -keyout ${KEY_FILE} -out ${CERT_FILE}
echo ">> Key written to:  ${KEY_FILE}"
echo ">> Cert written to: ${CERT_FILE}"

echo "> Updating sdk config"
python3 ${SCRIPT_PATH}/configure_local.py -c ${CERT_FILE}
