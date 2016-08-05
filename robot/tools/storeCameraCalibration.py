#!/usr/bin/env python3

import sys, os, time, struct

sys.path.insert(0, os.path.join("tools"))
import robotInterface
Anki = robotInterface.Anki
RI = Anki.Cozmo.RobotInterface
AC = Anki.Cozmo

class CameraCalibStorer:
    def onConnect(self, dest):
        "Callback when connected to the robot"
        print("Sending configuration to robot")
        robotInterface.Send(RI.EngineToRobot(writeNV=Anki.Cozmo.NVStorage.NVStorageWrite(
            Anki.Cozmo.NVStorage.NVReportDest.ENGINE,
            True,
            True,
            False,
            Anki.Cozmo.NVStorage.NVEntryTag.NVEntry_Invalid,
            Anki.Cozmo.NVStorage.NVStorageBlob(Anki.Cozmo.NVStorage.NVEntryTag.NVEntry_CameraCalib, self.payload))))
        
    def onOpResult(self, msg):
        "Callback when receiving an operation result"
        if msg.report.tag != Anki.Cozmo.NVStorage.NVEntryTag.NVEntry_CameraCalib:
            print("Result for unexpected tag 0x{:x}, {:d}".format(msg.tag, msg.report.result))
        else:
            if msg.report.result == Anki.Cozmo.NVStorage.NVResult.NV_OKAY:
                print("Success :-)")
            else:
                print("Failure :-(  {:d}".format(msg.report.result))
            robotInterface.Disconnect()
            sys.exit()
        
    def __init__(self, params):
        "Store the specified parameters into the robot"
        if type(params) is bytes:
            self.payload = params
        else:
            self.payload = AC.CameraCalibration(*params).pack()
        robotInterface.Init()
        robotInterface.SubscribeToConnect(self.onConnect)
        robotInterface.SubscribeToTag(RI.RobotToEngine.Tag.nvResult, self.onOpResult)
        robotInterface.Connect()

if __name__ == "__main__":
    print("Camera Calibration parameter utility")
    if len(sys.argv)-1 == len(AC.CameraCalibration.__slots__):
        try:
            params = [eval(a) for a in sys.argv[1:]]
        except:
            sys.exit("Couldn't evaluate arguments as parameters for CameraCalibration struct")
    elif len(sys.argv) > 1 and os.path.isfile(sys.argv[1]):
        params = open(sys.argv[1], 'rb').read()
    else:
        params = [eval(input("{} >>> ".format(s[1:]))) for s in AC.CameraCalibration.__slots__]
    print("Storing calibration parameters to robot, please wait.")
    CameraCalibStorer(params)
    time.sleep(5)
