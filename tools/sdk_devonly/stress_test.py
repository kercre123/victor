#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

import argparse
import threading
from threading import Thread
import random
import cozmo
import time
import os, json
import functools

from cozmo import _clad

'''Dev-only script, change cube colors and play animation to stress the connection
'''

_options = None
_cubes = set()
CUBE_OBJECT_FAMILY = 2

#Clad message to get information about what cubes are connected to this cozmo
def handle_available_objects(evt, *, msg):
    global _cubes
    _cubes = set()
    for availableObject in msg.objects:
        if availableObject.objectFamily == CUBE_OBJECT_FAMILY:
            _cubes.add(availableObject.objectID)

#Setting the clad message information to the correct color and settings
def _set_light(msg, idx, light):
    msg.onColor[idx] = light.on_color.int_color
    msg.offColor[idx] = light.off_color.int_color
    msg.onPeriod_ms[idx] = light.on_period_ms
    msg.offPeriod_ms[idx] = light.off_period_ms
    msg.transitionOnPeriod_ms[idx] = light.transition_on_period_ms
    msg.transitionOffPeriod_ms[idx] = light.transition_off_period_ms

#Sets all three cubes to the colors specified
def set_all_cube_lights(color, robot, sdk_conn):
    global _cubes
    for cube in _cubes:
        msg = cozmo._clad._clad_to_engine_iface.SetAllActiveObjectLEDs(
                objectID=cube, robotID=robot.robot_id)
        for j in range(4):
            _set_light(msg, j, color)
        sdk_conn.send_msg(msg)

#Randomly go through all the colors setting the cube colors to them
def change_colors(robot, sdk_conn):
    colors = [cozmo.lights.red_light,
              cozmo.lights.green_light,
              cozmo.lights.blue_light,
              cozmo.lights.white_light]
    random.shuffle(colors)
    for color in colors:
        set_all_cube_lights(color, robot, sdk_conn)
        robot.set_all_backpack_lights(color)

#Run the small win animation
#To be ran in a thread
def run_win_anim(robot, delay):
    while True:
        robot.play_anim("anim_keepaway_wingame_03").wait_for_completed()
        time.sleep(delay)

#Run the change cube colors
#To be ran in a thread
def run_change_colors(robot, sdk_conn, delay):
    while True:
        change_colors(robot, sdk_conn)
        time.sleep(delay)


def run(sdk_conn):
    global _options

    robot = sdk_conn.wait_for_robot()

    if not _options.anim_only:
        robot.add_event_handler(_clad._MsgAvailableObjects, handle_available_objects)
        get_block_pool_msg = cozmo._clad._clad_to_engine_iface.RequestAvailableObjects(False)
        sdk_conn.send_msg(get_block_pool_msg)
        Thread(target = run_change_colors, args=(robot, sdk_conn, float(_options.delay))).start()

    if not _options.lights_only:
        Thread(target = run_win_anim, args=(robot,float(_options.delay))).start()

    while True:
        time.sleep(1)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--delay', 
                        dest='delay',
                        default=0,
                        help='Time delay between animations and cube flashes in ms')
    parser.add_argument('-a', '--animation-only',
                        dest='anim_only',
                        default=False,
                        action='store_const',
                        const=True,
                        help='Run only the animation')
    parser.add_argument('-l', '--lights-only',
                        dest='lights_only',
                        default=False,
                        action='store_const',
                        const=True,
                        help='Run only the cube light change')

    _options = parser.parse_args()

    cozmo.setup_basic_logging()
    cozmo.connect(run)
