REM Tunnel a TCP connection TO_PORT:FROM_PORT
REM TO_PORT:   SDK_ON_DEVICE_TCP_PORT   - on app running on attached iOS device over USB
REM FROM_PORT: SDK_ON_COMPUTER_TCP_PORT - on local PC
REM Note: Ports can be the same number - they're on different devices
REM First remove any adb forwarding on this port (you can't connect to iOS and Android on same port at the same time)

python3 tcprelay.py 5106:5106
