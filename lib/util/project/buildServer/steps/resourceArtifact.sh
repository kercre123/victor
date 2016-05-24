set -e

GIT=`which git`
if [ -z $GIT ];then
  echo git not found
  exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

cd $TOPLEVEL/resources
OBJROOT=$TOPLEVEL/project/iOS/DerivedData

# create folder
if [ ! -d $OBJROOT/artifact ]; then
  mkdir -p $OBJROOT/artifact
fi


if [ -f resources.tar.gz ]; then rm -f resources.tar.gz; fi
tar czf resources.tar.gz config firmware/vehicle.ota sound
mv resources.tar.gz $OBJROOT/artifact/resources.tar.gz

