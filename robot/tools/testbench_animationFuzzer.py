#!/usr/bin/env python3
"""
Testbench script for fuzzing the robot's animation controller in various ways to try and produce crashes
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, time
sys.path.insert(0, os.path.join("tools"))
try:
    import robotInterface
except ImportError:
    sys.exit("Couldn't import robotInterface library. Are you running from :/robot ?")
else:
    from animationStreamer import *
    Anki = robotInterface.Anki
    RI = robotInterface.RI

def test_backpack():
    class BackpackStreamer(AnimationStreamer):
        def getMore(self, available):
            bplm = Anki.Cozmo.RobotInterface.EngineToRobot(animBackpackLights=Anki.Cozmo.AnimKeyFrame.BackpackLights())
            sil  = Anki.Cozmo.RobotInterface.EngineToRobot(animAudioSilence=Anki.Cozmo.AnimKeyFrame.AudioSilence())
            while self.bufferAvailable > (len(bplm.pack()) + len(bplm.pack())):
                self.send(bplm)
                self.send(sil)
                sys.stdout.write(str(time.time()) + '\r')
    streamer = BackpackStreamer()
    while True:
        robotInterface.Step()

if __name__ == '__main__':
    tests = [e for e in dir() if e.startswith('test_')]
    
    if len(sys.argv) < 2 or sys.argv[1] not in tests:
        sys.exit("""
Usage:
    {0} <TEST>
Where <TEST> is one of
 * {1}
""".format(sys.argv[0], "\n * ".join(tests)))
    
    robotInterface.Init(True, forkTransportThread = False)
    robotInterface.Connect(imageRequest=True)

    eval(sys.argv[1])()
