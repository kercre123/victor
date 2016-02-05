#!/usr/bin/env python3

import sys, os, time, struct

sys.path.insert(0, os.path.join("tools"))
import robotInterface
Anki = robotInterface.Anki
RI = Anki.Cozmo.RobotInterface

class PairedObjectsStorer:
    def onConnect(self, dest):
        "Callback when connected to the robot"
        print("Sending configuration to robot")
        nvCmd = Anki.Cozmo.NVStorage.NVStorageBlob(Anki.Cozmo.NVStorage.NVEntryTag.NVEntry_PairedObjects, self.pairedObjs.pack())
        robotInterface.Send(RI.EngineToRobot(writeNV=nvCmd))
        
    def onOpResult(self, msg):
        "Callback when receiving an operation result"
        if msg.tag != Anki.Cozmo.NVStorage.NVEntryTag.NVEntry_PairedObjects:
            print("Result for unexpected tag 0x{:x}, {:d}".format(msg.tag, msg.result))
        else:
            if msg.result == Anki.Cozmo.NVStorage.NVResult.NV_OKAY:
                print("Success :-)")
            else:
                print("Failure :-(  {:d}".format(msg.result))
            robotInterface.Disconnect()
            sys.exit()
        
    def __init__(self, params):
        "Store the specified parameters into the robot"
        self.pairedObjs = Anki.Cozmo.CubeSlots(params)
        robotInterface.Init()
        robotInterface.SubscribeToConnect(self.onConnect)
        robotInterface.SubscribeToTag(RI.RobotToEngine.Tag.nvResult, self.onOpResult)
        robotInterface.Connect()

if __name__ == "__main__":
    print("Paired Objects parameter utility")
    params = [eval(a) for a in sys.argv[1:]]
    print("Storing object parameters on robot, please wait.")
    PairedObjectsStorer(params)
    time.sleep(5)
