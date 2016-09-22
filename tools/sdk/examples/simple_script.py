#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

import asyncio
import cozmo

'''This script is made to show a bunch of interesting one liners you can do with Cozmo.
He will set his backpack lights to red
Play an angry animation
Drive forward for 3 seconds
Play a react to cliff animation
Then say the word "hello"
'''

def run(coz_conn):
    coz = coz_conn.wait_for_robot()
    
    coz.set_all_backpack_lights(cozmo.lights.red_light)

    coz.play_anim_trigger(cozmo.anim.Triggers.CubePounceLoseSession).wait_for_completed()

    coz.drive_wheels(50,50,50,50, duration=3)

    coz.play_anim_trigger(cozmo.anim.Triggers.ReactToCliff).wait_for_completed()

    coz.say_text("Hello").wait_for_completed()


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
