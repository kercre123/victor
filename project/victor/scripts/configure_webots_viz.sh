#!/bin/bash

# Go to directory of this script                                                                    
SCRIPT_PATH=$(dirname $([ -L $0 ] && echo "$(dirname $0)/$(readlink -n $0)" || echo $0))            
GIT=`which git`                                                                                     
if [ -z $GIT ]                                                                                      
then                                                                                                
    echo git not found                                                                              
    exit 1                                                                                          
fi                                                                                                  
TOPLEVEL=`$GIT rev-parse --show-toplevel`

source ${TOPLEVEL}/project/victor/scripts/android_env.sh

DEVICE_IP=$($ADB shell ifconfig wlan0 | awk /"inet addr"/'{print $2}' | awk -F: '{printf("%s",$2);}')
echo "DEVICE_IP = $DEVICE_IP"

WEBOTS_WORLD="${TOPLEVEL}/simulator/worlds/cozmo2Viz.wbt"

# modify webots world in-place
sed -i '' "s/\# forcedRobotIP \"192.168.X.X\"/forcedRobotIP \"$DEVICE_IP\"/" $WEBOTS_WORLD
sed -i '' "s/\# engineIP      \"192.168.X.X\"/engineIP      \"$DEVICE_IP\"/" $WEBOTS_WORLD
