#!/usr/bin/env python3
import asyncio
import math
import os
import sys

sys.path.append(os.path.join(os.path.dirname(os.path.dirname(os.getcwd())),'sdk/vector-sdk'))
import vector
from vector.util import degrees

class Lift:
  async def wait_async(self, t):
    return await asyncio.sleep(t)

  def SetLiftHeight(self, robot, cube):
    count = 0.0
    visible = False
    #setting robot lift to 0 each time as motors are not exact in moving by a little bit each time
    while not visible:
      count +=.005
      robot.behavior.set_lift_height(0.0, 1.0, 3.0)
      robot.behavior.set_lift_height(count, 1.0, 3.0)
      robot.loop.run_until_complete(self.wait_async(1))
      visible = cube._is_visible
      height =  robot.lift_height_mm
      while visible:
        print("HEIGHT: " + height)
      if count > 1.0:
        return 0
    return height

  def LiftHeightTest(self, robot, cube):
    passed = 0
    total = 5
    # realHeight =  figure this out
    heights = []
    robot.behavior.set_head_angle(degrees(0.0))
    robot.behavior.set_lift_height(0.0)
    for i in range(total):
      robotHeight = self.SetLiftHeight(robot, cube)
      rangeMin = realHeight - 0.1
      rangeMax = realHeight + 0.1
      if robotHeight >= rangeMin and robotHeight <= rangeMax:
        passed += 1
      heights.append(robotHeight)
    liftHeightResultTuple = (passed, total)
    robot.behavior.set_head_angle(degrees(0.0))
    robot.behavior.set_lift_height(0.0)
    return liftHeightResultTuple, heights, realHeight
