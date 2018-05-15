#
# This file is intended to be included in other scripts that need to communicate with the robot
#

#
# SSH/SCP support
#

: ${ANKI_ROBOT_USER:="root"}

robot_set_host ()
{
    if [ -z ${ANKI_ROBOT_HOST+x} ]; then
        GIT_PROJ_ROOT=`git rev-parse --show-toplevel`
        ROBOT_IP_FILE="${GIT_PROJ_ROOT}/robot_ip.txt"
        if [ ! -f $ROBOT_IP_FILE ]; then
            echo "ERROR: Missing file $ROBOT_IP_FILE"
            echo "You can create this file manually"
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
    ssh ${ANKI_ROBOT_USER}@${ANKI_ROBOT_HOST} $*
    return $?
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

    scp ${ARGS} ${SRC} ${DST}
    return $?
}

robot_cp_from ()
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

    SRC=$ANKI_ROBOT_USER@$ANKI_ROBOT_HOST:"$1"
    DST="$2"

    scp ${ARGS} ${SRC} ${DST}
    return $?
}
