#!/bin/bash
# Tunnel a TCP connection to:from
# to:   port 5106 (SDK_ON_DEVICE_TCP_PORT)   on app running on attached iOS device over USB
# from: port 5107 (SDK_ON_COMPUTER_TCP_PORT) on local PC
./tcprelay.py 5106:5107
