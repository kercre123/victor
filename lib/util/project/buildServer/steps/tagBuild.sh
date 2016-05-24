set -e

USERNAME=$1
PASSWORD=$2
BUILDID=$3

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi

SERVER_NAME="192.168.2.3"
PORT="8111"
Commit=`$GIT rev-parse --short HEAD`

set +e
echo curl -v --basic --request POST --header "Content-Type:text/plain" --data "$Commit" --user $USERNAME:$PASSWORD http://$SERVER_NAME:$PORT/httpAuth/app/rest/builds/id:$BUILDID/tags
curl -v --basic --request POST --header "Content-Type:text/plain" --data "$Commit" --user $USERNAME:$PASSWORD http://$SERVER_NAME:$PORT/httpAuth/app/rest/builds/id:$BUILDID/tags
set -e
