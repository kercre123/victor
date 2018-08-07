#!/usr/bin/env python3
import math
import os
import subprocess
import sys
import test_tread_motors as treadTest
import time
import utilities

sys.path.append(os.path.join(os.path.dirname(os.path.dirname(os.getcwd())),'sdk/vector-sdk'))
import vector
from vector.util import degrees, distance_mm, speed_mmps

BATTERY_LEVEL_LOW = 1
BATTERY_LEVEL_FULL = 3

def Main():
  args = utilities.parse_args()
  robot = vector.Robot(args.name, args.ip, str(args.cert), port=args.port)
  robot.connect()

  robot.behavior.set_head_angle(degrees(0.0))
  robot.world.disconnect_cube()
  connectionResult = robot.world.connect_cube()
  connected_cubes = robot.world.connected_light_cubes
  cube = connected_cubes[0]
  cube.set_light_corners(vector.lights.blue_light,
                         vector.lights.green_light,
                         vector.lights.blue_light,
                         vector.lights.green_light)

  treads = treadTest.Treads()
  driveTestTuple = treads.TreadTest(robot, cube)
  print(driveTestTuple)
  cube.set_lights_off()

def LowBatteryCheck(robot):
  batteryLevel = int(robot.get_battery_state().battery_level)
  while batteryLevel < BATTERY_LEVEL_FULL:
    time.sleep(300.0)
    batteryLevel = int(robot.get_battery_state().battery_level)

def GotoCharger(robot):
  onCharger = False
  charging = False
  while not onCharger and not charging:
    robot.behavior.drive_straight(distance_mm(-80), speed_mmps(10))
    time.sleep(5.0)
    onCharger = robot.get_battery_state().is_on_charger_platform
    charging = robot.get_battery_state().is_charging

def Reset(robot):
    GotoCharger()
    LowBatteryCheck()
    subprocess.call("./restart_vector.sh", shell = True)
    robot.connect()


if __name__ == "__main__":
  Main()
