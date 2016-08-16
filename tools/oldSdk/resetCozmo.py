#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time


cozmo = CozmoInterface()

cozmo.MoveHead(-1)
cozmo.MoveLift(-1)
time.sleep(3)
cozmo.Stop()
time.sleep(1)
cozmo.Shutdown()