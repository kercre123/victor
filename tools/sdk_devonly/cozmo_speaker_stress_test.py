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

'''Stress test for Cozmo

Make Cozmo run some specific animations endlessly, using for stress testing.
'''

import cozmo
import argparse
import sys

import time
from time import strftime
from datetime import datetime, timedelta

anim_arg = ""
volume_arg = ""

def parse_arguments(args):
  parser = argparse.ArgumentParser()
  
  parser.add_argument('-ac', '--anim_choice', action='store', 
                      help="List of anims or 'all', separated by a comma", required=True)

  parser.add_argument('-v', '--volume', action='store', help="Robot volume", required=False, default="1.0")
  
  options = parser.parse_args(args)
  return (parser, options)

def calculate_duration(start_time, end_time):
    datetime_format = "%Y-%m-%d %H:%M:%S"
    start = datetime.strptime(start_time, datetime_format)
    end = datetime.strptime(end_time, datetime_format)
    return end - start

def cozmo_program(robot: cozmo.robot.Robot):
  volume = float(volume_arg)
  robot.set_robot_volume(volume)
  anim_list = []
  if anim_arg == "all":
    anim_list = robot.conn.anim_names
  else:
    for anim_name in anim_arg.split(","):
      if anim_name in robot.conn.anim_names:
        anim_list.append(anim_name)
      else:
        print("\n\nAnimation {} is not valid. Please enter a valid animation and try again\n\n".format(anim_name))
        return
  start_time = strftime("%Y-%m-%d %H:%M:%S")
  while True:
    for anim in anim_list:
      print("Playing: {}...".format(anim))
      robot.play_anim(anim).wait_for_completed()
      print("Animation was played")
      time.sleep(2)
      end_time = strftime("%Y-%m-%d %H:%M:%S")
      duration = calculate_duration(start_time, end_time)
      print("Total time audio played : {}".format(str(duration)))

if __name__ == '__main__':
  args = sys.argv[1:]
  parser, options = parse_arguments(args)
  anim_arg = options.anim_choice
  volume_arg = options.volume

  cozmo.run_program(cozmo_program)
