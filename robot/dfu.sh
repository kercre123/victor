#!/bin/sh

# Updates the Victor robot firmware on a connected android device
#
# Must first build the syscon firmware using
#  > cd robot
#  > ./vmake.sh
#

# Go to directory of this script
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
GIT=`which git`
if [ -z $GIT ]
then
    echo git not found
    exit 1
fi
TOPLEVEL=`$GIT rev-parse --show-toplevel`

cd "${SCRIPT_PATH}"

ADB=adb
if [ -e $TOPLEVEL/local.properties ]; then
    ANDROID_HOME=`egrep sdk.dir $TOPLEVEL/local.properties | awk -F= '{print $2;}'`
    ADB=$ANDROID_HOME/platform-tools/adb
    if [ ! -x $ADB ]; then
        ADB=adb
    fi
fi

# List connect devices
$ADB devices

# Grab the IDs of all the connected devices / emulators
IDS=($($ADB devices | sed '1,1d' | sed '$d' | cut -f 1 | sort))
NUMIDS=${#IDS[@]}

# Check for number of connected devices / emulators
if [[ 0 -eq "$NUMIDS" ]]; then
    # No IDs, exit
    echo "No android devices detected - nothing to do."
    exit -1;
fi

ISROOT=`$ADB root`
if [[ ${#ISROOT} -lt 31 ]]; then
    echo "Running userinit"
    $ADB shell -x "./data/local/tmp/userinit.sh"
fi

cd hal/dfu
make
#make the remote schtuff?
$ADB push dfu /data/local/tmp
cd ../..
$ADB push syscon/build/syscon.dfu /data/local/tmp

# Shell into android device and execute dfu process
$ADB shell -x "
export LD_LIBRARY_PATH=/data/local/tmp
cd /data/local/tmp
./dfu syscon.dfu

function cleanup() {
  # Remount rootfs read-only
  [[ "$MOUNT_STATE" == "ro" ]] && logv "remount ro /" &&  robot_sh "/bin/mount -o remount,ro /"    
}

# trap ctrl-c and call ctrl_c()
trap cleanup INT

set +e

echo "Uploading dfu files"
robot_sh mkdir -p ${STAGING_DIR}
robot_cp ${DFU_PROGRAM} ${STAGING_DIR}
robot_cp ${DFU_PAYLOAD} ${STAGING_DIR}

DFU_PROGRAM_BASENAME=$(basename $DFU_PROGRAM)
DFU_PAYLOAD_BASENAME=$(basename $DFU_PAYLOAD)

echo "Running DFU"
NUM_RETRIES=5

# NOTE: DVT3 bootloaders have a bug that makes DFU fail often
# thus the retries. It also never gets further than "requesting validation"
# but this seems to be a sufficient enough indicator that the DFU has
# succeeded.
while [ $NUM_RETRIES -ge 0 ]; do
    status=$(/usr/bin/expect << EOF
        spawn ssh ${ANKI_ROBOT_USER}@${ANKI_ROBOT_HOST}
        expect "~ #"
        send "${STAGING_DIR}/${DFU_PROGRAM_BASENAME} ${STAGING_DIR}/${DFU_PAYLOAD_BASENAME}\n"
        set timeout 5
        expect {
            "requesting validation" {
                send_user "DFU_SUCCESS"
            }
            timeout {
                send_user "DFU_FAILED"
            }
        }
EOF
    )

    logv "RESULT: ${status}"
    if [[ $status = *"DFU_SUCCESS"* ]]; then
      echo "DFU SUCCESS"
      break
    fi

    let NUM_RETRIES=NUM_RETRIES-1
    if [ $NUM_RETRIES -ge 0 ]; then
      echo "Retrying... (Num retries left: ${NUM_RETRIES})"
    else
      echo "DFU FAILED"
      break
    fi
done # end while

cleanup