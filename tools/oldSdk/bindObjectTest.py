#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time

cozmo = CozmoInterface()

cozmo.DefineCustomObject("STAR5_Box", 100,200,100)
# cozmo.DefineCustomObject("ARROW_Box", 100,100,100)
while True:
    time.sleep(.1)

cozmo.Shutdown()
