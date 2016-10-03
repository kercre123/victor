#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
'''Make Cozmo turn toward a face.

This script shows off the turn_towards_face action. It will wait for a face
and then constantly turn towards it to keep it in frame.
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
        robot.turn_towards_face(face).wait_for_completed()
        time.sleep(.1)

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    try:
        cozmo.connect(run)
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)
