#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time, math

"""This is a script to create an imaginary wall in front of cozmo and
ask him to drive to the other side of it. Uses his path planning 
to navigate around the wall"""

cozmo = CozmoInterface()

cozmo.CreateFixedCustomObjectRelativeToRobotPose((200,0,0), 0, 25,150,50)

cozmo.DriveDistance(300,0,0, duration = 5)

cozmo.Shutdown()

