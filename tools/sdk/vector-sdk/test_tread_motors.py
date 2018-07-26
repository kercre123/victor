#!/usr/bin/env python3

import math
import numpy as np
import os
import subprocess
import sys
import time
# import utilities
# sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector
from vector.util import degrees, distance_inches, speed_mmps


args = utilities.parse_args()
robot = vector.Robot(args.name, args.ip, str(args.cert), port=args.port)

def Main():
  robot.behavior.drive_off_charger()
  # connect to cube
  connectionResult = robot.world.connect_cube()
  connected_cubes = robot.world.connected_light_cubes
  cube = connected_cubes[0]
  distance = Distance(cube)
  print("starting distance: " + str(distance) + " inches")
  Drive(6)
  distance = Distance(cube)
  print("long forward distance: " + str(distance) + " inches")
  Drive(-5)
  distance = Distance(cube)
  print("long backward distance: " + str(distance) + " inches")
  Drive(1)
  distance = Distance(cube)
  print("short forward distance: " + str(distance) + " inches")
  Drive(-1)
  distance = Distance(cube)
  print("short backward distance: " + str(distance) + " inches")


    
def Distance(cube):
  object_vector = np.array((cube.pose.position.x - robot.pose.position.x,
                            cube.pose.position.y - robot.pose.position.y))
  distance = (math.sqrt((object_vector**2).sum()))/25.4 #converts from mm to inches
  return distance

def Drive(inches):
  robot.behavior.drive_straight(distance_inches(inches), speed_mmps(15))
  time.sleep(5.0)

def LowBatteryCheck(): # check after completing a test
  return
  #check current battery level
  #if low, go to charge
  # charge for 20 min

def Reset():
  onCharger = False
  charging = False
  while not onCharger and not charging:
    robot.behavior.drive_on_charger()
    onCharger = robot.get_battery_state().is_on_charger_platform
    charging = robot.get_battery_state().is_charging
  LowBatteryCheck()
  subprocess.call("./restart_vector.sh")
  #subprocess.call("./restart_vector.sh", shell=True)

if __name__ == "__main__":
  Main()
