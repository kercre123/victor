#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time, math

""" This is a script for cozmo to drive in a square. He will drive 
forwards, left, back, and right. This is relative to his current positions
so if you move him, he will not account for that when he plans the next movement.
This uses path planning to get to the points so it accounts for the need to turn"""

cozmo = CozmoInterface(False, 0)

cozmo.StartSim()

cozmo.EnableReactionaryBehaviors(False)
time.sleep(1)

#Relative to cozmo
while(True):
    cozmo.DriveDistance(100,0,math.pi/2)
    time.sleep(5)

    cozmo.DriveDistance(0,100,math.pi/2)
    time.sleep(5)

    cozmo.DriveDistance(-100,0,math.pi/2)
    time.sleep(5)

    cozmo.DriveDistance(0,-100,math.pi/2)
    time.sleep(5)

cozmo.Stop()
time.sleep(1)
cozmo.Shutdown()
