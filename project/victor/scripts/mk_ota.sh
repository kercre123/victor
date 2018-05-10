#!/usr/bin/env bash
## Anki folder deploy OTA file generator
# Daniel Casner <daniel@anki.com>
#

set -e
set -u
SCRIPT_NAME=$(basename ${0})

function usage() {
  echo "$SCRIPT_NAME [OPTIONS]"
  echo "options:"
  echo "  -h                      print this message"
  echo "  -v                      verbose logging"
  echo "  -s                      Start HTTP server for OTA file"
  echo ""
}

SERVE=false
while getopts "hvs" opt; do
  case $opt in
    h)
        usage && exit 0
        ;;
    v)
        set -x
        ;;
    s)
        SERVE=true
        ;;
    *)
        usage && exit 1
        ;;
  esac
done


BUILD="_build/ota"
STAGING=${BUILD}/anki
BUILDSRC="_build/android/Release"
VICOSSRC="_build/vicos/Release"
ANKI_TAR=anki.tar.gz
MANIFEST=${BUILD}/manifest.ini
MAN_SIG=${BUILD}/manifest.sha256
: ${OTAKEY=project/victor/ota_test.key}

GZIP_MODE=--best
GZIP_WBITS=31

: ${UPDATE_VERSION=$(cat ${BUILDSRC}/etc/version)}

project/victor/scripts/install.sh ${BUILDSRC} ${BUILD}
project/victor/scripts/install.sh -k ${VICOSSRC} ${BUILD}
pushd ${BUILD}
COPYFILE_DISABLE=1 tar czf $ANKI_TAR anki
ANKI_BYTES=`wc -c $ANKI_TAR | awk '{ print $1 }'`
ANKI_SHA=`shasum -a256 -b $ANKI_TAR | awk '{ print $1 }'`
popd

echo "[META]"                           > $MANIFEST
echo "manifest_version=0.9.3"           >> $MANIFEST
echo "update_version=${UPDATE_VERSION}" >> $MANIFEST
echo "num_images=1"                     >> $MANIFEST
echo "[ANKI]"                           >> $MANIFEST
echo "delta=0"                          >> $MANIFEST
echo "compression=gz"                   >> $MANIFEST
echo "wbits=${GZIP_WBITS}"              >> $MANIFEST
echo "bytes=${ANKI_BYTES}"              >> $MANIFEST
echo "sha256=${ANKI_SHA}"               >> $MANIFEST

openssl dgst -sha256 -sign ${OTAKEY} -out $MAN_SIG $MANIFEST

pushd ${BUILD}
COPYFILE_DISABLE=1 tar cf victor-${UPDATE_VERSION}.ota manifest.ini manifest.sha256 $ANKI_TAR

if [ $SERVE == true ]; then
  python -m SimpleHTTPServer 5555
fi

popd
