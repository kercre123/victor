#! /bin/bash

: ${IP_ADDRESS:="${ANKI_ROBOT_HOST}"}

if [ -z "$PROJECT_DIR" ]; then
  PROJECT_DIR=`git rev-parse --show-toplevel`
fi

if [ -z "$AUTOTEST_DIR" ]; then
  AUTOTEST_DIR="${PROJECT_DIR}/_build/vicos/Release/auto-test/bin"
fi

if [ -z "$AUTOTEST_TOOL_DIR" ]; then
  AUTOTEST_TOOL_DIR="${PROJECT_DIR}/tools/automated-testing"
fi

if [ -z "$ROBOT_AUTOTEST_DIR" ]; then
  ROBOT_AUTOTEST_DIR="/auto-test"
fi

if [ -z "$ROBOT_AUTOTEST_BIN_DIR" ]; then
  ROBOT_AUTOTEST_BIN_DIR="/auto-test/bin"
fi

if [ -z "$VEC_CLI_PATH" ]; then
  VEC_CLI_PATH="$PROJECT_DIR/apps/demos/ble-pairing/vector-web-setup/node-client/vec-cli"
fi

if [ -z "$JENKINS_USER" ] || [ -z "$JENKINS_PASSWORD" ] || [ -z "$JENKINS_URL" ]; then
  JENKINS_UPLOAD=0
  echo "Warning: JENKINS_USER, JENKINS_PASSWORD, and JENKINS_URL must be set to upload TAP output results."
else 
  JENKINS_UPLOAD=1
fi

function getRandomString() {
  OUT=`head /dev/urandom | LC_ALL=C tr -dc A-Za-z0-9 | head -c16`
  echo $OUT
}

function getpath() {
  local path=""
  case $1 in
    /*) 
      path=$1
      ;;
    *)
      path=$AUTOTEST_TOOL_DIR/$1
      ;;
  esac
  echo $path
}

while getopts ":hr:t:c:k:n:o:x:i:" opt; do
  case ${opt} in
    r )
      IP_ADDRESS=$OPTARG
      ;;
    t )
      TEST_FILE=`getpath $OPTARG`
      ;;
    c )
      CONFIG_FILE=`getpath $OPTARG`
      ;;
    k )
      PIN=$OPTARG
      ;;
    n )
      NAME=$OPTARG
      ;;
    o )
      OUTPUT_PATH=`getpath $OPTARG`
      ;;
    x )
      DO_CLEAR_USER_DATA=1
      ;;
    i )
      ITERATIONS=$OPTARG
      ;;
    h )
      echo "Vector Automated Testing Help"
      echo "-h                help"
      echo "-r                *robot IP address"
      echo "-t                *JS test filepath"
      echo "-c                *config filepath"
      echo "-n                robot NAME"
      echo "-k                robot PIN"
      echo "-o                TAP test output location"
      echo "-x                clear robot user data periodically"
      echo "-i                test iterations"
      echo "(* required option)"
      exit 0
      ;;
    \? )
      echo "Invalid option: $OPTARG" 1>&2
      ;;
    : )
      echo "Invalid option: $OPTARG requires an argument" 1>&2
      ;;
  esac
done

if [ -z "$IP_ADDRESS" ]; then
  echo "Either supply IP address with '-r' option or set ANKI_ROBOT_HOST"
  exit 1
fi

if [ -z "$PIN" ]; then
  REGENERATE_NAME="TRUE"
fi

if [ -z "$ITERATIONS" ]; then
  ITERATIONS=1
fi

# Set PIN
if [ "$REGENERATE_NAME" == "TRUE" ]; then
  OUTPUT=`$AUTOTEST_TOOL_DIR/set_pairing_pin.sh -r $IP_ADDRESS`
  IFS=$'\n' read -rd '' -a RESULT <<<"$OUTPUT"

  PIN=${RESULT[0]}
  NAME=${RESULT[1]}
  echo "=> Generated name and pin: $NAME, $PIN"
fi

IFS=$' ' read -rd '' -a RESULT <<<"$NAME"
NAME_CODE=${RESULT[1]:0:4}

# Test Vector connectivity. If unavailable, attempt to put Vector on WiFi
ping -c 2 -t 2 $IP_ADDRESS > /dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "=> Could not connect to Vector over SSH."
  echo "=> Attempting to Put Vector on WiFi."
  $VEC_CLI_PATH                       \
    -f $NAME_CODE                     \
    -t $AUTOTEST_TOOL_DIR/wifi.js \
    -c $AUTOTEST_TOOL_DIR/config.json \
    -k $PIN;
fi

ping -c 2 -t 2 $IP_ADDRESS > /dev/null 2>&1
if [ $? -ne 0 ]; then
  echo "=> Wifi connection failed. Verify Vector's IP address or config settings."
  exit 2
fi

# mount as read/write
ssh root@$IP_ADDRESS "mount -o remount,rw /"

if [ $? -ne 0 ]; then
  exit $?
fi

# deploy binaries
ssh root@${IP_ADDRESS} "mkdir -p $ROBOT_AUTOTEST_BIN_DIR"
scp -rp $AUTOTEST_DIR root@${IP_ADDRESS}:/auto-test > /dev/null 2>&1

# mount as read only
ssh root@${IP_ADDRESS} "mount -o remount,ro /"

echo "=> Deployed test binaries"

if [ "$REGENERATE_NAME" == "TRUE" ]; then
  ssh root@${IP_ADDRESS} "reboot" &
  echo "=> Rebooting"
fi

######################################################

START=1
END=$ITERATIONS
for (( i=$START; i<=$END; i++))
do
  echo "=> iteration [$i]"

  #
  # if nth iteration, clear user data and reboot
  #
  ITER_CYCLE=$(($i % 10))
  if [[ $ITER_CYCLE -eq 1 && ($DO_CLEAR_USER_DATA -eq 1)]]; then
    echo "=> Clearing user data and rebooting..."
    ssh root@${IP_ADDRESS} "echo 1 > /run/wipe-data && reboot" &
    sleep 15
  fi
  #
  # run node-client tests
  #

  OPT_OUTPUT=""

  if [[ ! -z "$OUTPUT_PATH" ]]; then
    TAP_FILE_PATH="$OUTPUT_PATH/$(getRandomString).tap"
    OPT_OUTPUT="-o $TAP_FILE_PATH"
  fi

  $VEC_CLI_PATH                       \
    -f $NAME_CODE                     \
    -t $AUTOTEST_TOOL_DIR/autoTest.js \
    -c $AUTOTEST_TOOL_DIR/config.json \
    -k $PIN                           \
    $OPT_OUTPUT;

  # Upload test results
  if [ $JENKINS_UPLOAD -eq 1 ]; then
    CRUMB_JSON=`curl -s -u $JENKINS_USER:$JENKINS_PASSWORD http://$JENKINS_URL/crumbIssuer/api/json`
    CRUMB_HEADER=Jenkins-Crumb:`echo $CRUMB_JSON | cut -c62-93`

    curl -X POST $JENKINS_URL/job/sample/build \
          --user $JENKINS_USER:$JENKINS_PASSWORD \
          --header $CRUMB_HEADER \
          --form file0=@$TAP_FILE_PATH \
          --form json='{"parameter": [{"name":"upload.txt", "file":"file0"}]}'
  fi

done

######################################################

# mount as read/write
ssh root@$IP_ADDRESS "mount -o remount,rw /"
# remove binaries
ssh root@${IP_ADDRESS} "rm -rf $ROBOT_AUTOTEST_DIR"
# mount as read only
ssh root@${IP_ADDRESS} "mount -o remount,ro /"