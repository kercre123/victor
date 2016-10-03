#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
'''Tell Cozmo to drive to the specified pose and orientation.

Define a destination pose for Cozmo. If relative_to_robot is set to true,
the given pose will assume the robot's pose as its origin.
'''

import sys

import cozmo
from cozmo.util import degrees, Pose


def run(sdk_conn):
    '''The run method runs once Cozmo is connected.'''
    robot = sdk_conn.wait_for_robot()

    robot.go_to_pose(Pose(100, 100, 0, angle_z=degrees(45)), relative_to_robot=True)

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    try:
        cozmo.connect(run)
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)
