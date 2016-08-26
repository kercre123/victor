REM Forward a tcp connection tcp:LOCAL_PORT tcp:REMOTE_PORT
REM REMOTE_PORT: SDK_ON_DEVICE_TCP_PORT   - on app running on attached Android device over USB
REM LOCAL_PORT:  SDK_ON_COMPUTER_TCP_PORT - on local PC
REM Note: Ports can be the same number - they're on different devices
REM       This requires that an Android device is attached (otherwise you'll see "error: no devices/emulators found")
REM       The forward is removed by adb whenever the device is lost, so you must rerun the script again

REM remove existing adb forward, then bind, then list to verify it succeeded
adb forward --remove tcp:5106
adb forward --no-rebind tcp:5106 tcp:5106
adb forward --list