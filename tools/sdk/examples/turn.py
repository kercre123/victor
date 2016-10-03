#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
'''Turn Cozmo and play an animation.

Turn the robot around its current position by 90 degrees. Start the MajorWin animation trigger playing on Cozmo.
'''

import sys

import cozmo
from cozmo.util import degrees


def run(sdk_conn):
    '''The run method runs once Cozmo is connected.'''
    robot = sdk_conn.wait_for_robot()

    # Turn 90 degrees, play an animation, exit.
    robot.turn_in_place(degrees(90)).wait_for_completed()

    anim = robot.play_anim_trigger(cozmo.anim.Triggers.MajorWin)
    anim.wait_for_completed()


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    try:
        cozmo.connect(run)
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)
