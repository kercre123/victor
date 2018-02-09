#!/bin/bash

set -e

SCRIPT_NAME=`basename ${0}`
GIT=`which git`
if [ -z $GIT ]; then
  echo git not found
  exit 1
fi
: ${TOPLEVEL:=`$GIT rev-parse --show-toplevel`}

function usage() {
    echo "$SCRIPT_NAME -d [DIR] [OPTIONS]"
    echo "  -h                      print this message"
    echo "  -d [DIR]                directory with generated targets to process (REQUIRED)"
}

#
# defaults
#
DIR=""

while getopts ":d:h" opt; do
    case $opt in
        h)
            usage
            exit 1
            ;;
        d)
            DIR="${OPTARG}"
            ;;
        :)
            echo "Option -${OPTARG} required an argument." >&2
            usage
            exit 1
            ;;
    esac
done

# Move past getops args
shift $(($OPTIND - 1))

if [ -z "${DIR}" ]; then
	echo "-d option is required." >&2
	usage
	exit 1
fi

shopt -s nullglob
for GODIR in ${DIR}/*.godir.lst
do
	GOPATH=${TOPLEVEL}/$(cat $DIR/$(basename $GODIR .godir.lst).gopath.lst)
	# GODIR must be relative; switch to TOPLEVEL to execute
	# GOPATH must be absolute
	(cd $TOPLEVEL; GOPATH=$GOPATH CGO_ENABLED=1 $GOROOT/bin/go get -d -v $(cat $GODIR) )
done
