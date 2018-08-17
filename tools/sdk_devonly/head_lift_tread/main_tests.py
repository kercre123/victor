#!/usr/bin/env python3
import asyncio
import os
import subprocess
import sys
import test_tread_motors as treadTest
import head_test as headTest
import time

sys.path.append(os.path.join(os.path.dirname(os.path.dirname(os.getcwd())),'sdk/vector-sdk'))
import vector

from vector.util import degrees, distance_mm, speed_mmps

BATTERY_LEVEL_LOW = 1
BATTERY_LEVEL_FULL = 3
CUBE_CONNECT_RETRY = 3
TIME_LIMIT = 30

try:
    TREAD_CUBE_ADDR = os.environ['TREAD_CUBE_ADDR']
except KeyError:
    print("Please set the environment variable TREAD_CUBE_ADDR")
try:
    HEAD_CUBE_ADDR = os.environ['HEAD_CUBE_ADDR']
except KeyError:
    print("Please set the environment variable HEAD_CUBE_ADDR")
try:
    LIFT_CUBE_ADDR = os.environ['LIFT_CUBE_ADDR']
except KeyError:
    print("Please set the environment variable LIFT_CUBE_ADDR")

def Main():
  args = vector.util.parse_test_args()
  robot = vector.Robot(args.name, args.ip, str(args.cert), port="443", show_viewer=True)
  robot.connect()
  robot.world.disconnect_cube()
  robot.behavior.set_head_angle(degrees(0.0))
  robot.behavior.set_lift_height(0.0)

  cube = ConnectCube(robot, TREAD_CUBE_ADDR)
  if cube != 0:
    treads = treadTest.Treads()
    driveTestTuple = treads.TreadTest(robot, cube)
  robot.behavior.set_head_angle(degrees(0.0))
  robot.behavior.set_lift_height(0.0)

  cube = ConnectCube(robot, HEAD_CUBE_ADDR)
  if cube != 0:
    head = headTest.Head()
    headAngleTuple, angles, realAngle = head.HeadAngleTest(robot, cube)

  TurnOffDisconnectCube(robot, cube)


async def wait_async(t):
      return await asyncio.sleep(t)

def ConnectCube(robot, address):
  cubeConnected = True
  countConnect = 0
  countDisconnect = 0
  connected_cubes = robot.world.connected_light_cubes

  #set correct cube connection bool
  if len(connected_cubes) == 0:
    cubeConnected = False
  else: #turn off cube lights and disconnect from cube if connected
    cubeConnected = True
    connected_cubes[0].set_lights_off()
    robot.world.disconnect_cube()


  #make sure cube is disconnected as it can take a while
  while cubeConnected:
    connected_cubes = robot.world.connected_light_cubes
    if len(connected_cubes) == 0:
      cubeConnected = False
      break;
    robot.loop.run_until_complete(wait_async(5))
    countDisconnect +=5
    if countDisconnect >= TIME_LIMIT:
      break;
  #change prefered cube so robot will connect to differenct cube
  robot.world.forget_preferred_cube()
  robot.world.set_preferred_cube(address)
  connectionResult = robot.world.connect_cube()
  #make sure robot connects to a cube within time limit
  while cubeConnected == False:
    connected_cubes = robot.world.connected_light_cubes
    if len(connected_cubes) != 0:
      cubeConnected = True
      break
    robot.loop.run_until_complete(wait_async(5))
    countConnect +=5
    if countConnect >= TIME_LIMIT:
      break;

  connected_cubes = robot.world.connected_light_cubes
  if len(connected_cubes) != 0:
    cube = connected_cubes[0]
    if cube._factory_id == address:
      cube.set_light_corners(vector.lights.blue_light,
                             vector.lights.green_light,
                             vector.lights.blue_light,
                             vector.lights.green_light)
      return cube
  else:
    return 0

def GotoCharger(robot):
  charging = False
  while not charging:
    robot.behavior.drive_straight(distance_mm(-1), speed_mmps(2))
    time.sleep(1.0)
    charging = robot.get_battery_state().is_charging

def LowBatteryCheck(robot):
  batteryLevel = int(robot.get_battery_state().battery_level)
  while batteryLevel < BATTERY_LEVEL_FULL:
    time.sleep(300.0)
    batteryLevel = int(robot.get_battery_state().battery_level)

def Reset(robot):
    GotoCharger()
    LowBatteryCheck()
    subprocess.call("./restart_vector.sh", shell = True)
    robot.connect()

def TurnOffDisconnectCube(robot, cube):
  cubeConnected = True
  connected_cubes = robot.world.connected_light_cubes
  if len(connected_cubes) == 0:
    cubeConnected = False
  else:
    cubeConnected = True
    cube.set_lights_off()
    robot.world.disconnect_cube()

  count = 0
  while cubeConnected:
    connected_cubes = robot.world.connected_light_cubes
    if len(connected_cubes) == 0:
      cubeConnected = False
    robot.loop.run_until_complete(wait_async(5))
    count +=5
    if count >=TIME_LIMIT:
      break

if __name__ == "__main__":
  Main()
