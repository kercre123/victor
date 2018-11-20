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

# if [ -e ${SCRIPT_PATH}/victor_env.sh ]; then
#     # This is so that this same dfu script can be used for 
#     # standalone syscon update packages containing victor_env.sh
#     # in the same folder.
#     source ${SCRIPT_PATH}/victor_env.sh
# else
#     echo "Error - missing victor_env.sh"
#     exit
# fi

cd "${SCRIPT_PATH}"

# Settings can be overridden through environment
: ${VERBOSE:=0}
: ${STAGING_DIR:="/anki/bin/dfu_staging"}
: ${ANKI_ROBOT_USER:="root"}

DFU_PROGRAM="${SCRIPT_PATH}/dfu"
DFU_PAYLOAD="${SCRIPT_PATH}/syscon.dfu"


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

function robot_set_host ()
{
    if [ -z ${ANKI_ROBOT_HOST+x} ]; then
        ROBOT_IP_FILE="${SCRIPT_PATH}/robot_ip.txt"
        if [ ! -f $ROBOT_IP_FILE ]; then
            echo "ERROR: Missing file $ROBOT_IP_FILE"
            echo "You can create this file manually, or supply the IP on the command line "
            echo "The file just needs to contain the robot's IP address"
            exit
        fi
        ANKI_ROBOT_HOST=$(cat $ROBOT_IP_FILE)
    fi
}

robot_sh ()
{
    if [ -z ${ANKI_ROBOT_HOST} ]; then
        echo "ERROR: Unspecified robot host."
        return 1
    fi
    #ssh ${ANKI_ROBOT_USER}@${ANKI_ROBOT_HOST} $*
    ssh -i ./id_rsa_victor_shared ${ANKI_ROBOT_USER}@${ANKI_ROBOT_HOST} $*
}

robot_cp ()
{
    if [ -z ${ANKI_ROBOT_HOST} ]; then
        echo "ERROR: Unspecified robot host"
        return 1
    fi

    if [ $# -eq 3 ]; then
        ARGS="$1"
        shift
    else
        ARGS=""
    fi

    SRC="$1"
    DST=$ANKI_ROBOT_USER@$ANKI_ROBOT_HOST:"$2"

    scp -i ./id_rsa_victor_shared ${ARGS} ${SRC} ${DST}
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
  echo "ERROR: DFU program not found. Invalid Installation."
  exit
fi
if [ ! -e ${DFU_PAYLOAD} ]; then
  echo "ERROR: DFU payload not found. Invalid Installation."
  exit
fi

echo "Stopping robot processes"
robot_sh systemctl stop anki-robot.target

echo "Checkig Mount State"
# Remount rootfs read-write if necessary
MOUNT_STATE=$(\
    robot_sh "grep ' / ext4.*\sro[\s,]' /proc/mounts > /dev/null 2>&1 && echo ro || echo rw"\
)
echo "mountstate is " $MOUNT_STATE
[[ "$MOUNT_STATE" != "rw" ]] && logv "remount rw /" && robot_sh "/bin/mount -o remount,rw /"

function cleanup() {
  # Remount rootfs read-only
  [[ "$MOUNT_STATE" == "ro" ]] && logv "remount ro /" &&  robot_sh "/bin/mount -o remount,ro /"    
  exit
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
        spawn ssh -i ./id_rsa_victor_shared ${ANKI_ROBOT_USER}@${ANKI_ROBOT_HOST}
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
