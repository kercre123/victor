#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time, math

"""This is an example of a simple script for cozmo. He plays an angry animation,
drives forward for 3 seconds and then plays a success animation"""

cozmo = CozmoInterface(False, 0)

cozmo.StartSim()
time.sleep(2)

cozmo.PlayAnimation("anim_speedTap_loseSession_03")

cozmo.DriveWheels(50,50,50,50)
time.sleep(3)
cozmo.Stop()
cozmo.PlayAnimation("majorWin")

cozmo.Stop()
time.sleep(1)
cozmo.Shutdown()
