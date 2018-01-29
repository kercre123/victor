# Helper functions for control (ctl) scripts

function exec_background()
{
    if [ $# -eq 0 ]; then
        echo "error: missing program to execute"
        exit 1
    fi

    EXE="$1"
    shift

    nohup logwrapper ${EXE} $* </dev/null >/dev/null 2>&1 &
}

function stop_process()
{
    if [ $# -eq 0 ]; then
        echo "error: missing program name"
        exit 1
    fi

    PROG_NAME="$1"

    PROG_PID=$(pidof ${PROG_NAME})
    PROG_RUNNING=$?

    if [ $PROG_RUNNING -eq 0 ]; then
        kill $PROG_PID
        echo "stopping ${PROG_PID}..."
    fi

    # Wait for process to actually die
    STOPPING_PID=$(pidof ${PROG_NAME}) 
    while [ ! -z $STOPPING_PID ]; do
      STOPPING_PID=$(pidof ${PROG_NAME}) 
    done

    echo "stopped ${PROG_PID}"
}

function process_status()
{
    if [ $# -eq 0 ]; then
        echo "error: missing program name"
        exit 1
    fi

    PROG_NAME="$1"

    PROG_PID=$(pidof ${PROG_NAME})
    PROG_RUNNING=$?

    if [ $PROG_RUNNING -eq 0 ]; then
        echo "running ${PROG_PID}"
    else
        echo "stopped"
    fi
}

function get_process_pid()
{
    PROG_NAME="$1"
    echo $(pidof ${PROG_NAME})
}

function get_process_name()
{
    echo $1
}

function usage()
{
    echo "$SCRIPT_NAME [COMMAND]"
    echo "start                 start $PROGRAM_NAME"
    echo "stop                  stop $PROGRAM_NAME"
    echo "restart               restart $PROGRAM_NAME"
    echo "status                report run status"
}

function main()
{
    ERROR=0
    case $ARGV in
        start)
            start_program
            ERROR=$?
            ;;
        stop)
            stop_program
            ERROR=$?
            ;;
        restart)
            stop_program && start_program
            ERROR=$?
            ;;
        status)
            program_status
            ERROR=$?
            ;;
        get_pid)
            get_program_pid
            ERROR=$?
            ;;
        get_name)
            get_program_name
            ERROR=$?
            ;;
        -h)
            usage
            ERROR=1
            ;;
        *)
            echo "unknown command: $ARGV"
            usage
            ERROR=1
            ;;
    esac

    exit $ERROR
}
