#!/bin/bash
# Tunnel a TCP connection TO_PORT:FROM_PORT
# TO_PORT:   SDK_ON_DEVICE_TCP_PORT   - on app running on attached iOS device over USB
# FROM_PORT: SDK_ON_COMPUTER_TCP_PORT - on local PC
# Note: Ports can be the same number - they're on different devices

# First remove any adb forwarding on this port (you can't connect to iOS and Android on same port at the same time)
adb forward --remove tcp:5106

./tcprelay.py 5106:5106
