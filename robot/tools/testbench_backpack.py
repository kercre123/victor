#!/usr/bin/env python3
"""
Python command line tool for playing with Cozmo's backpack
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, time, argparse
sys.path.insert(0, os.path.join("tools"))
try:
    import robotInterface
except ImportError:
    sys.exit("Couldn't import robotInterface library. Are you running from <engine>/robot ?")
Anki = robotInterface.Anki

def encodeColor(red, grn, blu, ir=False):
    if type(red) is float:
        red = round(red*255)
    if type(grn) is float:
        grn = round(grn*255)
    if type(blu) is float:
        blu = round(blu*255)
    ir = 1 if ir else 0
    return ((red >> 3) << Anki.Cozmo.LEDColorEncodedShifts.LED_ENC_RED_SHIFT) | \
           ((grn >> 3) << Anki.Cozmo.LEDColorEncodedShifts.LED_ENC_GRN_SHIFT) | \
           ((blu >> 3) << Anki.Cozmo.LEDColorEncodedShifts.LED_ENC_BLU_SHIFT) | \
           (ir << Anki.Cozmo.LEDColorEncodedShifts.LED_ENC_IR_SHIFT)

class BackpackLights(Anki.Cozmo.RobotInterface.BackpackLights):
    "Helper class for setting backpack lights"

    def SetLight(self, channel, red, green, blue):
        self.lights[channel].onColor  = encodeColor(red, green, blue)
        self.lights[channel].onFrames = 255
    
    def SetLeft(self, *rest):
        self.SetLight(Anki.Cozmo.LEDId.LED_BACKPACK_LEFT, *rest)
    def SetFront(self, *rest):
        self.SetLight(Anki.Cozmo.LEDId.LED_BACKPACK_FRONT, *rest)
    def SetMiddle(self, *rest):
        self.SetLight(Anki.Cozmo.LEDId.LED_BACKPACK_MIDDLE, *rest)
    def SetBack(self, *rest):
        self.SetLight(Anki.Cozmo.LEDId.LED_BACKPACK_BACK, *rest)
    def SetRight(self, *rest):
        self.SetLight(Anki.Cozmo.LEDId.LED_BACKPACK_RIGHT, *rest)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(prog="backpack",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--left',   nargs=3, type=int)
    parser.add_argument('--front',  nargs=3, type=int)
    parser.add_argument('--middle', nargs=3, type=int)
    parser.add_argument('--back',   nargs=3, type=int)
    parser.add_argument('--right',  nargs=3, type=int)
    parser.add_argument('--no-sync',   action='store_true', help="Do not send sync time message to robot on startup.")
    parser.add_argument('--sync-time', default=0, type=int, help="Manually specify sync time offset")
    parser.add_argument('-i', '--ip_address', default="172.31.1.1", help="Specify robot's ip address")
    parser.add_argument('-p', '--port', default=5551, type=int, help="Manually specify robot's port")
    args = parser.parse_args()

    robotInterface.Init()
    robotInterface.Connect(dest=(args.ip_address, args.port), syncTime = None if args.no_sync else args.sync_time)

    bp = BackpackLights()
    if args.left:
        bp.SetLeft(*args.left)
    if args.front:
        bp.SetFront(*args.front)
    if args.middle:
        bp.SetMiddle(*args.middle)
    if args.back:
        bp.SetBack(*args.back)
    if args.right:
        bp.SetRight(*args.right)

    try:
        while not robotInterface.GetConnected():
            time.sleep(0.005)
        while True:
            robotInterface.Send(Anki.Cozmo.RobotInterface.EngineToRobot(setBackpackLights=bp))
            time.sleep(1)
    except KeyboardInterrupt:
        robotInterface.Disconnect()
        sys.exit()
