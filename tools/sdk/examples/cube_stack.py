#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
'''Make Cozmo stack Cubes.

This script is meant to show off how easy it is to do high level robot actions.
Cozmo will wait until he sees two Cubes, and then will pick up one and place it on the other.
He will pick up the first one he sees, and place it on the second one.
'''

import sys

import cozmo

def run(sdk_conn):
    '''The run method runs once Cozmo is connected.'''
    robot = sdk_conn.wait_for_robot()

    lookaround = robot.start_behavior(cozmo.behavior.BehaviorTypes.LookAroundInPlace)

    cubes = robot.world.wait_until_observe_num_objects(num=2, object_type=cozmo.objects.LightCube, timeout=60)

    lookaround.stop()

    if len(cubes) < 2:
        print("Error: need 2 Cubes but only found", len(cubes), "Cube(s)")
    else:
        robot.pickup_object(cubes[0]).wait_for_completed()
        robot.place_on_object(cubes[1]).wait_for_completed()

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    try:
        cozmo.connect(run)
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)
