#!/usr/bin/env bash

set -e

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
ALL_FILES=1

function usage() {
    echo "$SCRIPT_NAME [OPTIONS]"
    echo "  -h  print this message"
    echo "  -a  include all files (include examples)"
    echo "  -v  verbose output"
}

while getopts "hva" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        a)
            ALL_FILES=
            ;;
        v)
            set -x
            ;;
    esac
done

FILES="$(find ${SCRIPT_PATH}/../vector-sdk/anki_vector -type f -d 1 -name "*.py")"
if [ -z "${ALL_FILES}" ]; then
    FILES="$FILES $(find ${SCRIPT_PATH}/../vector-sdk/examples -type f -name "*.py")"
fi

python3 -c "import autopep8" || { echo; echo "You must install autopep8 first with 'pip3 install autopep8'" ; exit 1; }
echo "> Running autopep8"
python3 -m autopep8 --ignore E501,E402 --in-place ${FILES}

python3 -c "import pylint" || { echo; echo "You must install pylint first with 'pip3 install pylint'" ; exit 1; }
echo "> Running pylint"
python3 -m pylint --rcfile="${SCRIPT_PATH}/.pylint" ${FILES}
