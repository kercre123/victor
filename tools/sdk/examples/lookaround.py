#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

import asyncio

import cozmo
from cozmo.util import degrees


def run(coz_conn):
    coz = coz_conn.wait_for_robot()

    look_around = coz.start_behavior(cozmo.behavior.BehaviorTypes.LookAround)

    # try to find a block
    cube = None

    try:
        cube = coz.world.wait_for_observed_light_cube(timeout=30)
        print("Found cube", cube)

    except asyncio.TimeoutError:
        print("Didn't find a cube :-(")

    finally:
        # whether we find it or not, we want to stop the behavior
        look_around.stop()

    if cube is None:
        coz.play_anim_trigger(cozmo.anim.Triggers.MajorFail)
        return

    print("Yay, found cube")

    cube.set_lights(cozmo.lights.green_light.flash())

    anim = coz.play_anim_trigger(cozmo.anim.Triggers.BlockReact)
    anim.wait_for_completed()


    action = coz.pickup_object(cube)
    print("got action", action)
    result = action.wait_for_completed(timeout=30)
    print("got action result", result)

    coz.turn_in_place(degrees(90)).wait_for_completed()

    action = coz.place_object_on_ground_here(cube)
    print("got action", action)
    result = action.wait_for_completed(timeout=30)
    print("got action result", result)

    anim = coz.play_anim_trigger(cozmo.anim.Triggers.MajorWin)
    cube.set_light_corners(None, None, None, None)
    anim.wait_for_completed()


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
