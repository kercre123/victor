#!/usr/bin/env python3
import asyncio
import math
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector
from vector.util import degrees

class Head:
  async def wait_async(self, t):
    return await asyncio.sleep(t)

  def RealAngle(self): #pythagorean theorem to find angle, all numbers in mm
    #125.36mm=(dist from head center gear to lift front) + (cube length *2) + (space between robot lift and cube)
    adjacentDist =  40.36 + 44.0 + 44.0 + 1
     #94mm=(camera view height when image is first visible) - (dist form center head gear to treads)
    oppositeDist = 100 - 50
    toa = math.degrees(math.atan(oppositeDist/adjacentDist))
    return toa

  def SetHeadAngle(self, robot, cube):
    count = 10.0
    visible = False
    #setting robot head to 0 each time as motors are not exact in moving by one degree each time
    while not visible:
      count +=1
      robot.behavior.set_head_angle(degrees(0.0), 2.0, 4.0)
      robot.behavior.set_head_angle(degrees(count), 2.0, 4.0)
      robot.loop.run_until_complete(self.wait_async(1))
      visible = cube._is_visible
      deg =  math.degrees(robot.head_angle_rad)
      if count > 45:
        return 0
    return deg

  def HeadAngleTest(self, robot, cube):
    robot.viewer.show_video()
    passed = 0
    total = 5
    realAngle = self.RealAngle()
    angles = []
    for i in range(total):
      robotAngle = self.SetHeadAngle(robot, cube)
      rangeMin = realAngle - 2.5
      rangeMax = realAngle + 2.5
      if robotAngle >= rangeMin and robotAngle <= rangeMax:
        passed += 1
      angles.append(robotAngle)
    headAngleResultTuple = (passed, total)
    return headAngleResultTuple, angles, realAngle
