#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
'''Make Cozmo perform different actions based on the number of Cubes he finds.

This script shows off simple decision making.
It tells Cozmo to look around, and then wait until he sees a certain amount of objects.
Based on how many object he sees before he times out, he will do different actions.
0-> be angry
1-> roll block (the block must not be face up)
2-> stack blocks (the blocks must all be face up)
'''

import sys

import cozmo


def run(coz_conn):
    '''The run method runs once Cozmo is connected.'''
    coz = coz_conn.wait_for_robot()

    lookaround = coz.start_behavior(cozmo.behavior.BehaviorTypes.LookAroundInPlace)

    cubes = coz.world.wait_until_observe_num_objects(num=2, object_type=cozmo.objects.LightCube, timeout=10)

    print(cubes)

    lookaround.stop()

    if len(cubes) == 0:
        coz.play_anim_trigger(cozmo.anim.Triggers.MajorFail).wait_for_completed()
    elif len(cubes) == 1:
        coz.run_timed_behavior(cozmo.behavior.BehaviorTypes.RollBlock, active_time=60)
    else:
        coz.run_timed_behavior(cozmo.behavior.BehaviorTypes.StackBlocks, active_time=60)

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    try:
        cozmo.connect(run)
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)
