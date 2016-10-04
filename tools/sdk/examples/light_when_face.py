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


def light_when_face(robot):
    '''The core of the light_when_face program'''

    face = None

    while True:
        if face and face.is_visible:
            robot.set_all_backpack_lights(cozmo.lights.blue_light)
        else:
            robot.set_backpack_lights_off()

            # Wait until we we can see another face
            try:
                face = robot.world.wait_for_observed_face(timeout=30)
            except asyncio.TimeoutError:
                print("Didn't find a face.")
                return

        time.sleep(.1)


def run(sdk_conn):
    '''The run method runs once the Cozmo SDK is connected.'''
    robot = sdk_conn.wait_for_robot()

    try:
        light_when_face(robot)

    except KeyboardInterrupt:
        print("")
        print("Exit requested by user")


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    try:
        cozmo.connect_with_tkviewer(run)
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)
