#!/bin/bash
# Forward a tcp connection tcp:LOCAL_PORT tcp:REMOTE_PORT
# REMOTE_PORT: SDK_ON_DEVICE_TCP_PORT   - on app running on attached Android device over USB
# LOCAL_PORT:  SDK_ON_COMPUTER_TCP_PORT - on local PC
# Note: Ports can be the same number - they're on different devices
#       This requires that an Android device is attached (otherwise you'll see "error: no devices/emulators found")
#       The forward is removed by adb whenever the device is lost, so you must rerun the script again
adb forward tcp:5106 tcp:5106
adb forward --list
