set -e

echo "cladTest.sh: Entering directory /vagrant"
cd /vagrant

# PYTHON=2.7 or 3
PYTHON=$1

if [ "${1}" == "3" ]; then
    PYTHON_VERSION="PYTHON=python${PYTHON}"
else
    PYTHON_VERSION=""
fi
# build
make $PYTHON_VERSION OUTPUT_DIR=build -C emitters/tests

EXIT_STATUS=$?
set -e

# exit
exit $EXIT_STATUS
