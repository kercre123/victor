#!/usr/bin/env python3
"""
Python command line tool for echoing messages coming from the Cozmo robot to the command line
"""
__author__ = "Bryon Vandiver <bryon@anki.com>"

import sys, os, time, argparse
sys.path.insert(0, os.path.join("tools"))
try:
    import robotInterface
except ImportError:
    sys.exit("Couldn't import robotInterface library. Are you running from <engine>/robot ?")

global previous, batch

previous = 0
tags = robotInterface.RI.RobotToEngine.Tag
batch = []

class TapPrinter:
    "Class for printing tap messages to console"

    COLORS = [      
        (0xFF, 0xFF, 0xFF),
        (0xFF, 0xFF,    0),
        (0xFF,    0,    0),
        (0xFF,    0, 0xFF),
        (   0,    0, 0xFF),
        (   0,    0,    0),
        (   0, 0xFF,    0),
        (   0, 0xFF, 0xFF),
    ]

    def encodeColors(self, r, g, b):
        return ((r >> 3) * 0x400) | ((g >> 3) * 0x20) | (b >> 3)

    def updateCubeColor(self, id):
        target = robotInterface.Anki.Cozmo.CubeID(objectID=id, rotationPeriod_frames=10)
        robotInterface.Send(robotInterface.RI.EngineToRobot(setCubeID=target))

        taps, axis = self.slotTaps[id], self.slotAxis[id]

        t_c = self.encodeColors(*self.COLORS[taps % len(self.COLORS)])
        a_c = self.encodeColors(*self.COLORS[axis % len(self.COLORS)])

        lights = robotInterface.Anki.Cozmo.CubeLights(lights=[
                robotInterface.Anki.Cozmo.LightState(onColor=t_c, offColor=t_c),
                robotInterface.Anki.Cozmo.LightState(onColor=t_c, offColor=t_c),
                robotInterface.Anki.Cozmo.LightState(onColor=a_c, offColor=a_c),
                robotInterface.Anki.Cozmo.LightState(onColor=a_c, offColor=a_c)
            ])
        robotInterface.Send(robotInterface.RI.EngineToRobot(setCubeLights=lights))

        gamma = robotInterface.Anki.Cozmo.SetCubeGamma(gamma=0x7F)
        robotInterface.Send(robotInterface.RI.EngineToRobot(setCubeGamma=gamma))

    def discovered(self, msg):
        if msg.rssi > 60:
            return

        for slot, id in enumerate(self.slotTaken):
            if id and id != msg.factory_id:
                continue

            self.slotTaken[slot] = msg.factory_id
            
            grab = robotInterface.Anki.Cozmo.SetPropSlot(factory_id=msg.factory_id, slot=slot)
            robotInterface.Send(robotInterface.RI.EngineToRobot(setPropSlot=grab))
            return

    def axisChanged(self, msg):
        self.slotAxis[msg.objectID] = msg.upAxis

    def printTap(self, msg):
        global batch
        batch += [msg]

    def printConnect(self, msg):
        if not msg.connected:
            self.slotTaken[msg.objectID] = None
        else:
            self.updateCubeColor(msg.objectID)

        print (repr(msg))
    
    def printFiltered(self, msg):
        self.slotTaps[msg.objectID] += 1
        self.updateCubeColor(msg.objectID)
        print (repr(msg))
    
    def __init__(self, args):
        self.slotTaken = [None] * 4
        self.slotTaps = [0] * 4
        self.slotAxis = [0] * 4


        robotInterface.SubscribeToTag(tags.activeObjectUpAxisChanged, self.axisChanged)
        robotInterface.SubscribeToTag(tags.objectTappedFiltered, self.printFiltered)
        robotInterface.SubscribeToTag(tags.activeObjectDiscovered, self.discovered)
        robotInterface.SubscribeToTag(tags.activeObjectConnectionState, self.printConnect)
        robotInterface.SubscribeToTag(tags.activeObjectTapped, self.printTap)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(prog="tapTest",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-i', '--ip_address', default="172.31.1.1", help="Specify robot's ip address")
    parser.add_argument('-p', '--port', default=5551, type=int, help="Manually specify robot's port")
    args = parser.parse_args()

    robotInterface.Init()
    robotInterface.Connect(dest=(args.ip_address, args.port),
                           syncTime = 0,
                           imageRequest = False,
                           blinkers = False)

    TapPrinter(args)
    try:
        while True:
            if len(batch) <= 0:
                time.sleep(0.03)
            else:
                time.sleep(0.1)
                
                print ("=======================")
                for msg in sorted(batch, key=lambda msg: msg.tapPos - msg.tapNeg, reverse=True):
                    print ("{} {:>8}: {}".format(msg.objectID, msg.tapPos - msg.tapNeg, repr(msg)))
                batch = []  

    except KeyboardInterrupt:
        robotInterface.Die()
