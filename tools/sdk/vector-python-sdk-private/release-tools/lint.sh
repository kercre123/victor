#!/usr/bin/env bash

set -e

SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
EXAMPLE_FILES=
TEST_FILES=

function usage() {
    echo "$SCRIPT_NAME [OPTIONS]"
    echo "  -h  print this message"
    echo "  -a  include all files (include examples & tests)"
    echo "  -e  include all files (include examples)"
    echo "  -t  include all files (include tests)"
    echo "  -v  verbose output"
}

while getopts "hvaet" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        a)
            EXAMPLE_FILES=$(find ${SCRIPT_PATH}/../sdk/examples -type f -name "*.py")
            TEST_FILES=$(find ${SCRIPT_PATH}/../tests -d 1 -type f -name "*.py")
            ;;
        e)
            EXAMPLE_FILES=$(find ${SCRIPT_PATH}/../sdk/examples -type f -name "*.py")
            ;;
        t)
            TEST_FILES=$(find ${SCRIPT_PATH}/../tests -d 1 -type f -name "*.py")
            ;;
        v)
            set -x
            ;;
    esac
done

FILES="$(find ${SCRIPT_PATH}/../sdk/anki_vector -type f -name "*.py" ! -name '*_pb2.py' ! -name '*_pb2_grpc.py')"

python3 -c "import autopep8" || { echo; echo "You must install autopep8 first with 'pip3 install autopep8'" ; exit 1; }
echo "> Running autopep8"
python3 -m autopep8 --ignore E501,E402 --in-place ${FILES}
if [ ! -z "${EXAMPLE_FILES}" ]; then
    echo "> Running autopep8 on example files"
    python3 -m autopep8 --ignore E501,E402 --in-place ${EXAMPLE_FILES}
fi
if [ ! -z "${TEST_FILES}" ]; then
    echo "> Running autopep8 on test files"
    python3 -m autopep8 --ignore E501,E402 --in-place ${TEST_FILES}
fi

STATUS_CODE=0
python3 -c "import pylint" || { echo; echo "You must install pylint first with 'pip3 install pylint'" ; exit 1; }
echo "> Running pylint"
python3 -m pylint --rcfile="${SCRIPT_PATH}/.pylint" ${FILES} || STATUS_CODE=$?
if [ ! -z "${EXAMPLE_FILES}" ]; then
    echo "> Running pylint on example files"
    python3 -m pylint --rcfile="${SCRIPT_PATH}/.pylint" ${EXAMPLE_FILES} || STATUS_CODE=$?
fi
if [ ! -z "${TEST_FILES}" ]; then
    echo "> Running pylint on test files"
    python3 -m pylint --rcfile="${SCRIPT_PATH}/.pylint" --disable=W0621 ${TEST_FILES} || STATUS_CODE=$?
fi

exit ${STATUS_CODE}
