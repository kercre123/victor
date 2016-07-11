#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time, math

""" This is a script for cozmo to drive in a square. He will drive 
forwards, left, back, and right. This is relative to his current positions
so if you move him, he will not account for that when he plans the next movement.
This uses path planning to get to the points so it accounts for the need to turn"""

cozmo = CozmoInterface()

for i in range(4):
    cozmo.DriveWheels(50,50,50,50, duration = 3)
    cozmo.TurnInPlace(radians = math.pi/2, duration = 3.5)


cozmo.Shutdown()
