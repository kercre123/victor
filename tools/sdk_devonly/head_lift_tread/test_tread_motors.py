#!/usr/bin/env python3
import asyncio
import math
import os
import sys
import time

sys.path.append(os.path.join(os.path.dirname(os.path.dirname(os.getcwd())),'sdk/vector-sdk'))
import vector
from vector.util import distance_mm, speed_mmps

class Treads:
  async def wait_async(self, t):
      return await asyncio.sleep(t)

  #get the disance between the robot and the cube
  def CubeDistance(self, robot, cube):
    equalOriginId = False
    robot.loop.run_until_complete(self.wait_async(0.5))
    #make sure that both the robot and cube are anchored to the same absolute origin
    #so distance calculations are accurate
    while not equalOriginId:
      try:
        cubeOriginId = cube.pose.origin_id
        if robot.pose.origin_id == cubeOriginId:
          equalOriginId = True
      except:
        robot.loop.run_until_complete(self.wait_async(0.5))
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
    distDrive = [100, -70, 30, -40, -20]
    passedDrive = 0
    curCubeDist = self.CubeDistance(robot, cube)
    for dist in distDrive:
        self.Drive(robot, dist)
        cubeDist = self.CubeDistance(robot, cube)
        dist = math.fabs(curCubeDist-cubeDist)
        rangeMax =  abs(dist) + 4
        rangeMin = abs(dist) - 4
        if dist >= rangeMin and dist <= rangeMax:
            passedDrive += 1
    driveTestTuple = (passedDrive, 5)
    return driveTestTuple

