#!/usr/bin/env python3

# Copyright (c) 2016 Anki, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License in the file LICENSE.txt or at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

'''Run stress test Cozmo

Make Cozmo move head and speak.
'''

import cozmo
import argparse
import sys
import devices_util
import time
from time import strftime
from datetime import datetime, timedelta
from os import system
import platform

def parse_arguments(args):
  parser = argparse.ArgumentParser()
  
  parser.add_argument('-I', action='store_true', help="Set iOS flag to true")

  parser.add_argument('-A', action='store_true', help="Set Android flag to true")

  parser.add_argument('-S', '--serial_id', action='store', required=True,
                      help="Serial ID of connected phone")
  
  options = parser.parse_args(args)
  return (parser, options)

def calculate_duration(start_time, end_time):
    datetime_format = "%Y-%m-%d %H:%M:%S"
    start = datetime.strptime(start_time, datetime_format)
    end = datetime.strptime(end_time, datetime_format)
    return end - start

def cozmo_program(robot: cozmo.robot.Robot):
    current_platform = platform.system().lower()
    if current_platform == "windows":
      system("title {} - Running on Cozmo serial: {}".format(
              devices_util.DevicesInfo.get_device_name(serial), robot.serial))
    elif current_platform == "darwin":
      sys.stdout.write("\x1b]0;{} - Running on Cozmo serial: {} \x07".format(
                       devices_util.DevicesInfo.get_device_name(serial), robot.serial))
      
    robot.set_robot_volume(1.0)
    i = 0
    #robot.move_lift(-5)
    time.sleep(2)
    start_time = strftime("%Y-%m-%d %H:%M:%S")
    while True:
        i = i + 1
        print("index : {}".format(str(i)))
        # Step 1 : play anim anim_factory_audio_test_01
        robot.play_anim("anim_factory_audio_test_01").wait_for_completed()
        time.sleep(1)
        print("play done")
        # Step 2 : move the head down
        robot.move_head(-5)
        time.sleep(1)
        print("move the head down done")
        # Step 3 : move the head up
        robot.move_head(5)
        time.sleep(1)
        print("move the head up done")
        end_time = strftime("%Y-%m-%d %H:%M:%S")
        duration = calculate_duration(start_time, end_time)
        print("duration : {}".format(str(duration)))
        #break


if __name__ == '__main__':
  args = sys.argv[1:]
  parser, options = parse_arguments(args)

  serial = options.serial_id

  if options.I == True:
    connector = cozmo.run.IOSConnector(serial=serial)
  elif options.A == True:
    connector = cozmo.run.AndroidConnector(serial=serial)
  else:
    print("Doesn't seem to be any flags set. Try again")
  
  cozmo.run_program(cozmo_program, connector=connector)

