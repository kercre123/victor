#!/usr/bin/env python3
import asyncio
import math
import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import anki_vector
from anki_vector.util import degrees

class Head:
  async def wait_async(self, t):
    return await asyncio.sleep(t)

  def RealAngle(self): #pythagorean theorem to find angle, all numbers in mm
    #125.36mm=(dist from head center gear to lift front) + (cube length) + (space between robot lift and cube)
    # + (distance from cube platform to cube infront of robot)
    adjacentDist =  40.36 + 44.0 + 1 + 40
     #94mm=(height of cube platform) + (height of cube image) - (dist form center head gear to treads)
    oppositeDist =  (112.0 + 32.0) - 50
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
      robot.loop.run_until_complete(self.wait_async(0.5))
      visible = cube._is_visible
      deg =  math.degrees(robot.head_angle_rad)
      diff = abs(count - deg)
      if count > 45:
        return 0
    CameraAngleCheck(self)
    return deg

  def HeadAngleTest(self, robot, cube):
    passed = 0
    total = 5
    realAngle = self.RealAngle()
    for i in range(5):
      robotAngle = self.SetHeadAngle(robot, cube)
      rangeMin = realAngle - 2
      rangeMax = realAngle + 2
      if robotAngle >= rangeMin and robotAngle <= rangeMax:
        passed += 1
    headAngleTuple = (passed, total)
    return headAngleTuple
