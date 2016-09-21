#!/usr/bin/env python3
"""Single purpose utility to kill body code on a robot"""

import sys, os, time
sys.path.insert(0, os.path.join("tools"))
import robotInterface
Anki = robotInterface.Anki
RI = robotInterface.RI

class ESNGetter:
    
    def onConnect(self, connInfo):
        robotInterface.Send(RI.EngineToRobot(getMfgInfo=RI.GetManufacturingInfo()))
        
    def onRA(self, ra):
        self.head = ra.robotID
        
    def onMfg(self, mfg):
        self.body = mfg.esn
        print("ESNs: ", hex(self.head), hex(self.body))
    
    def __init__(self):
        self.head = 0
        self.body = 0
        robotInterface.SubscribeToConnect(self.onConnect)
        robotInterface.SubscribeToTag(RI.RobotToEngine.Tag.robotAvailable, self.onRA)
        robotInterface.SubscribeToTag(RI.RobotToEngine.Tag.mfgId, self.onMfg)

robotInterface.Init()
g = ESNGetter()
robotInterface.Connect()
time.sleep(5)
