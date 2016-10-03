#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
'''Make Cozmo drive in a square.

This script is designed to show off the 'simple robot capabilites' of Cozmo.
He will drive in a square by going forward and turning (left) 4 times.
'''

import sys

import cozmo
from cozmo.util import degrees

def run(sdk_conn):
    '''The run method runs once Cozmo is connected.'''
    robot = sdk_conn.wait_for_robot()

    for _ in range(4):
        robot.drive_wheels(50, 50, duration=3)
        robot.turn_in_place(degrees(90)).wait_for_completed()

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    try:
        cozmo.connect(run)
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)
