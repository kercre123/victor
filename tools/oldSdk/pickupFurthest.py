#!/usr/bin/env python3
from cozmoInterface import CozmoInterface
import time, math

cozmo = CozmoInterface()

cozmo.WaitUntilSeeBlocks(3, timeout = 30)

state = cozmo.GetState()

(dist,id) = (0,0)
for cube in state.lightCubes:
    (x,y,rad) = cozmo.GetPoseRelRobot(cube.pose.x, cube.pose.y, cube.topFaceOrientation_rad)
    if abs(x) > dist: (dist,id) = (abs(x), cube.objectID)

print(dist,id)
cozmo.PickupObject(state.cubeOrder.index(id))

cozmo.Shutdown()
