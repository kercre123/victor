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
import logging
import cozmo
import argparse
import sys
import devices_util
import time
from time import strftime
from datetime import datetime, timedelta
from os import system

logger = devices_util.Logger.get_logger(__name__)

def parse_arguments(args):
  parser = argparse.ArgumentParser()
  
  platform_group = parser.add_mutually_exclusive_group(required=True)
  platform_group.add_argument('-I', action='store_true', help="Set iOS flag to true")
  platform_group.add_argument('-A', action='store_true', help="Set Android flag to true")

  id_group = parser.add_mutually_exclusive_group(required=True)
  id_group.add_argument('-si', '--serial_id', action='store',
                      help="Serial ID of connected phone")
  id_group.add_argument('-ai', '--asset_id', action='store',
                      help="Asset Manager's ID of connected phone")

  parser.add_argument('-t', '--timeout', action='store', required=False,
                      default = '1440', help="Maximum time to run in minute")
  
  options = parser.parse_args(args)
  return (parser, options)

def calculate_duration(start_time, end_time):
    datetime_format = "%Y-%m-%d %H:%M:%S"
    start = datetime.strptime(start_time, datetime_format)
    end = datetime.strptime(end_time, datetime_format)
    return end - start

def cozmo_program(robot: cozmo.robot.Robot):
    global timeout 

    logger.info("{} - Running on Cozmo serial: {}".format(
                      devices_util.DevicesInfo().get_device_name(serial), robot.serial))
    robot.set_robot_volume(1.0)
    i = 0
    time.sleep(2)
    start_time = strftime("%Y-%m-%d %H:%M:%S")
    while timeout > 0:
        i = i + 1
        logger.info("index : {}".format(str(i)))
        # Step 1 : play anim anim_factory_audio_test_01
        robot.play_anim("anim_factory_audio_test_01").wait_for_completed()
        time.sleep(1)
        logger.info("play done")
        # Step 2 : move the head down
        robot.move_head(-5)
        time.sleep(1)
        logger.info("move the head down done")
        # Step 3 : move the head up
        robot.move_head(5)
        time.sleep(1)
        logger.info("move the head up done")
        end_time = strftime("%Y-%m-%d %H:%M:%S")
        duration = calculate_duration(start_time, end_time)
        logger.info("Duration : {}".format(str(duration)))
        timeout = timeout - duration.total_seconds() / 60.0

if __name__ == '__main__':
  global timeout

  args = sys.argv[1:]
  parser, options = parse_arguments(args)

  if options.serial_id:
    serial = options.serial_id
  else:
    serial = devices_util.DevicesInfo.get_serial_id(options.asset_id)
  timeout = float(options.timeout)

  if options.I == True:
    connector = cozmo.run.IOSConnector(serial=serial)
  elif options.A == True:
    connector = cozmo.run.AndroidConnector(serial=serial)
  
  cozmo.run_program(cozmo_program, connector=connector)

