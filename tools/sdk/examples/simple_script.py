#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
'''Control Cozmo with simple scripts.


This script is made to show a bunch of interesting one liners you can do with Cozmo.
He will set his backpack lights to red
Play an angry animation
Drive forward for 3 seconds
Play a react to cliff animation
Then say the word "hello"
'''

import sys

import cozmo

def run(sdk_conn):
    '''The run method runs once Cozmo is connected.'''
    robot = sdk_conn.wait_for_robot()

    robot.set_all_backpack_lights(cozmo.lights.red_light)

    robot.play_anim_trigger(cozmo.anim.Triggers.CubePounceLoseSession).wait_for_completed()

    robot.drive_wheels(50, 50, 50, 50, duration=3)

    robot.play_anim_trigger(cozmo.anim.Triggers.ReactToCliff).wait_for_completed()

    robot.say_text("Hello").wait_for_completed()


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    try:
        cozmo.connect(run)
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)
