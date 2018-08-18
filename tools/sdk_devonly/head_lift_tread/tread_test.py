#!/usr/bin/env python3
import asyncio
import math
import os
import sys
import time

sys.path.append(os.path.join(os.path.dirname(os.path.dirname(os.getcwd())),'sdk/vector-sdk'))
import vector
from vector.util import distance_mm, speed_mmps, degrees

class Treads:
  async def wait_async(self, t):
      return await asyncio.sleep(t)

  #get the disance between the robot and the cube
  def CubeDistance(self, robot, cube):
    equalOriginId = False
    robot.loop.run_until_complete(self.wait_async(0.5))
    count = 0
    #make sure that both the robot and cube are anchored to the same absolute origin
    #so distance calculations are accurate
    while not equalOriginId:
      try:
        cubeOriginId = cube.pose.origin_id
        if robot.pose.origin_id == cubeOriginId:
          equalOriginId = True
      except:
        robot.loop.run_until_complete(self.wait_async(0.5))
        count +=5
        if count >= 30:
          return 0
    distance = (math.sqrt(
               ((cube.pose.position.x - robot.pose.position.x) ** 2)
             + ((cube.pose.position.y - robot.pose.position.y) ** 2)
             ))- (22+29)
    return distance

  def Drive(self, robot, mm):
    robot.behavior.drive_straight(distance_mm(mm), speed_mmps(20))
    time.sleep(3.0)

  #get distance robot thinks it is, see if distance driven is how far robot actually went
  def TreadTest(self, robot, cube):
    distDrive = [120, -70, 50, -40, -10]
    passedDrive = 0
    distsTupleArray = []
    cubeDist = self.CubeDistance(robot, cube)
    robot.behavior.set_head_angle(degrees(0.0))
    robot.behavior.set_lift_height(0.0)
    if cubeDist != 0:
      for dist in distDrive:
          self.Drive(robot, dist)
          curCubeDist = self.CubeDistance(robot, cube)
          distDiff = math.fabs(curCubeDist-cubeDist)
          rangeMax =  abs(dist) + 4
          rangeMin = abs(dist) - 4
          if distDiff >= rangeMin and distDiff <= rangeMax:
              passedDrive += 1
          distsTupleArray.append(distDiff)
    driveTestTuple = (passedDrive, 5)
    robot.behavior.set_head_angle(degrees(0.0))
    robot.behavior.set_lift_height(0.0)
    return driveTestTuple, distsTupleArray

