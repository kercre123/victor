set -e

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi

Commit=`$GIT rev-parse --short HEAD`
echo "##teamcity[buildNumber '$Commit']"
