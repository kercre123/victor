#!/usr/bin/env python3

import sys, os, time

sys.path.insert(0, os.path.join("tools"))
import robotInterface
RI = robotInterface.RI
Anki = robotInterface.Anki

def onConnect(*args):
    robotInterface.Send(RI.EngineToRobot(imuRequest=Anki.Cozmo.IMURequest(0xFFFFFFFF)))
    
def onData(msg):
    sys.stdout.write("{timestamp}, {g[0]}, {g[1]}, {g[2]}, {a[0]}, {a[1]}, {a[2]}{linesep}".format(timestamp=msg.timestamp, g=msg.g, a=msg.a, linesep=os.linesep))

if __name__ == '__main__':
    robotInterface.Init()
    robotInterface.SubscribeToConnect(onConnect)
    robotInterface.SubscribeToTag(RI.RobotToEngine.Tag.imuRawDataChunk, onData)
    robotInterface.Connect()

    try:
        while True:
            sys.stdout.flush()
            time.sleep(0.03)
    except KeyboardInterrupt:
        robotInterface.Die()
