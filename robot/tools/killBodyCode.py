#!/usr/bin/env python3
"""Single purpose utility to kill body code on a robot"""

import sys, os, time
sys.path.insert(0, os.path.join("tools"))
import robotInterface
Anki = robotInterface.Anki
RI = robotInterface.RI

def sendKillCode(connInfo):
  robotInterface.Send(RI.EngineToRobot(killBodyCode=Anki.Cozmo.KillBodyCode()))

robotInterface.Init()
robotInterface.SubscribeToConnect(sendKillCode)
robotInterface.Connect()
time.sleep(5)
