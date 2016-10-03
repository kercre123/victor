#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
'''Wait for Cozmo to see a face, and then turn on his backpack light.

This is a script to show off faces, and how they are easy to use.
It waits for a face, and then will light up his backpack when that face is visible.
'''

import asyncio
import sys
import time

import cozmo


def run(sdk_conn):
    '''The run method runs once Cozmo is connected.'''
    robot = sdk_conn.wait_for_robot()

    try:
        face = robot.world.wait_for_observed_face(timeout=30)
    except asyncio.TimeoutError:
        print("Didn't find a face.")
        return

    while True:
        if face.is_face_visible():
            robot.set_all_backpack_lights(cozmo.lights.blue_light)
        else:
            robot.set_backpack_lights_off()
        time.sleep(.1)

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    try:
        cozmo.connect(run)
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)
