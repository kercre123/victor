#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time, math

"""This is a script to create an imaginary wall in front of cozmo and
ask him to drive to the other side of it. Uses his path planning 
to navigate around the wall"""

cozmo = CozmoInterface(False, 0)
cozmo.StartSim()
time.sleep(2)


# Example Code to try and navigate a maze (doesn't work)
# cozmo.CreateObjectRelRobot(100,0,0,50,500,50)
# cozmo.CreateObjectRelRobot(300,-100,0,50,300,50)
# cozmo.CreateObjectRelRobot(500,0,0,50,500,50)

# cozmo.CreateObjectRelRobot(300,250,math.pi/2,50,300,50)
# cozmo.CreateObjectRelRobot(400,-250,math.pi/2,50,100,50)
# cozmo.DriveDistance(400,-200,0)

cozmo.CreateObjectRelRobot(100,0,0,50,500,50)

time.sleep(1)

cozmo.DriveDistance(300,0,0)
time.sleep(10)

cozmo.PlayAnimation("majorWin")

cozmo.Stop()
time.sleep(1)
cozmo.Shutdown()
