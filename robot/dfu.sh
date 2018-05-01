#!/bin/sh
# Updates the syscon firmware on a networked Victor robot
#
# Must first build the syscon firmware using
#  > cd robot
#  > ./vmake.sh
#

set -e
set -u

# Go to directory of this script
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))
SCRIPT_NAME=$(basename ${0})
GIT=`which git`
if [ -z $GIT ]
then
    echo git not found
    exit 1
fi
: ${TOPLEVEL:=`$GIT rev-parse --show-toplevel`}

if [ -e ${SCRIPT_PATH}/victor_env.sh ]; then
    # This is so that this same dfu script can be used for 
    # standalone syscon update packages containing victor_env.sh
    # in the same folder.
    source ${SCRIPT_PATH}/victor_env.sh
else
    source ${SCRIPT_PATH}/../project/victor/scripts/victor_env.sh
fi

cd "${SCRIPT_PATH}"

# Settings can be overridden through environment
: ${VERBOSE:=0}
: ${STAGING_DIR:="/anki/bin/dfu_staging"}
DFU_PROGRAM="${SCRIPT_PATH}/hal/dfu/dfu"
DFU_PAYLOAD="${SCRIPT_PATH}/syscon/build/syscon.dfu"

function logv() {
  if [ $VERBOSE -eq 1 ]; then
    echo -n "[$SCRIPT_NAME] "
    echo $*;
  fi
}

function usage() {
  echo "$SCRIPT_NAME [OPTIONS]"
  echo "options:"
  echo "  -h                      print this message"
  echo "  -v                      print verbose output"
  echo "  -d DFU_PROGRAM          DFU program to use"
  echo "  -p DFU_PAYLOAD          syscon payload file to use"
  echo "  -s ANKI_ROBOT_HOST      hostname or ip address of robot"
}

while getopts "hvd:p:s:" opt; do
  case $opt in
    h)
      usage && exit 0
      ;;
    v)
      VERBOSE=1
      ;;
    d)
      DFU_PROGRAM="${OPTARG}"
      ;;
    p)
      DFU_PAYLOAD="${OPTARG}"
      ;;      
    s)
      ANKI_ROBOT_HOST="${OPTARG}"
      ;;
    *)
      usage && exit 1
      ;;
  esac
done

robot_set_host

if [ -z "${ANKI_ROBOT_HOST+x}" ]; then
  echo "ERROR: unspecified robot target. Pass the '-s' flag or set ANKI_ROBOT_HOST"
  usage
  exit 1
fi

echo "ANKI_ROBOT_HOST: ${ANKI_ROBOT_HOST}"

# Check that DFU files exist
if [ ! -e ${DFU_PROGRAM} ]; then
  echo "ERROR: DFU program not found. Run make from robot/hal/dfu/ to build."
  exit
fi
if [ ! -e ${DFU_PAYLOAD} ]; then
  echo "ERROR: DFU payload not found. Run vmake.sh to build"
  exit
fi

echo "Stopping robot processes"
robot_sh systemctl stop anki-robot.target

# Remount rootfs read-write if necessary
MOUNT_STATE=$(\
    robot_sh "grep ' / ext4.*\sro[\s,]' /proc/mounts > /dev/null 2>&1 && echo ro || echo rw"\
)
[[ "$MOUNT_STATE" == "ro" ]] && logv "remount rw /" && robot_sh "/bin/mount -o remount,rw /"

function cleanup() {
  # Remount rootfs read-only
  [[ "$MOUNT_STATE" == "ro" ]] && logv "remount ro /" &&  robot_sh "/bin/mount -o remount,ro /"    
}

# trap ctrl-c and call ctrl_c()
trap cleanup INT

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