#!/usr/bin/env bash

set -e

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
FILES="$(find ${SCRIPT_PATH}/../vector-sdk/examples -type f -name "*.py") $(find ${SCRIPT_PATH}/../vector-sdk/tests ${SCRIPT_PATH}/../vector-sdk/anki_vector -type f -d 1 -name "*.py")"

function usage() {
    echo "$SCRIPT_NAME [OPTIONS]"
    echo "  -h  print this message"
    echo "  -v  verbose output"
}

while getopts "hv" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        v)
            set -x
            ;;
    esac
done

python3 -c "import autopep8" || { echo; echo "You must install autopep8 first with 'pip3 install autopep8'" ; exit 1; }
echo "> Running autopep8"
python3 -m autopep8 --ignore E501,E402 --in-place ${FILES}

python3 -c "import pylint" || { echo; echo "You must install autopep8 first with 'pip3 install pylint'" ; exit 1; }
echo "> Running pylint"
python3 -m pylint --rcfile=.pylint ${FILES}
