#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
'''Define cuboids of a custom size and place them in the world.

This script will define two objects in front of cozmo, and let him observe the
star (Custom_STAR5_Box) marker. The arrow marker (Custom_ARROW_Box) may also be used.
'''

import time
import cozmo
from cozmo.util import degrees, Pose


def run(coz_conn):
    '''The run method runs once Cozmo is connected.'''
    coz = coz_conn.wait_for_robot()

    fixed_object = coz.world.create_custom_fixed_object(Pose(200, 0, 0, angle_z=degrees(0)),
                                                        10, 100, 200, relative_to_robot=True)
    if fixed_object:
        print("fixed_object created successfully")

    coz.world.define_custom_object(cozmo.objects.CustomObjectTypes.Custom_STAR5_Box, 10, 100, 200)

    time.sleep(100)

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
