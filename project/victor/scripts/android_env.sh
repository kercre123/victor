#
# This file is intended to be included in other scripts that require the ANDROID_SDK or ADB
#

ADB=adb                                                                                             
if [ -e $TOPLEVEL/local.properties ]; then                                                          
    ANDROID_HOME=`egrep sdk.dir $TOPLEVEL/local.properties | awk -F= '{print $2;}'`                 
    ADB=$ANDROID_HOME/platform-tools/adb                                                            
    if [ ! -x $ADB ]; then                                                                          
        ADB=adb                                                                                     
    fi                                                                                              
fi                                                                                                  

# echo "ADB: $ADB"

function adb_shell {
    CMD='$ADB'
    CMD+=" shell '"
    CMD+="$1; echo "
    CMD+='$?'
    CMD+=" | tr -d [:space:]"
    CMD+="'"

    # uncomment for debugging   
    # set -x
    RC=$(eval $CMD)
    # set +x
    # echo "RC = $RC"

    [[ $RC == "0" ]]
}
                                                                                                    
# Grab the IDs of all the connected devices / emulators                                             
IDS=($($ADB devices | sed '1,1d' | sed '$d' | cut -f 1 | sort))
NUMIDS=${#IDS[@]}
                                                                                                    
# Check for number of connected devices / emulators                                                 
if [[ 0 -eq "$NUMIDS" ]]; then                                                                      
    # No IDs, exit                                                                                  
    echo "No android devices detected - nothing to do."                                             
    exit -1                                                                                        
elif [[ "$NUMIDS" -gt 1 ]]; then
    if [ -z ${ANDROID_SERIAL+x} ]; then 
        echo "Multiple devices detected - set ANDROID_SERIAL env variable to select a device"
        exit -1
    fi
fi
