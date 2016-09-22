#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

import asyncio
import time
import cozmo
from cozmo.util import degrees, Pose

'''This script will define an object in front of cozmo, and let him observe the star and arrow markers.
you can see this in the viz window of the app.'''

def run(coz_conn):
    coz = coz_conn.wait_for_robot()
    
    fixed_object = coz.world.create_custom_fixed_object(Pose(200,0,0,angle_z=degrees(0)), 10, 100, 200, relative_to_robot=True)
    coz.world.define_custom_object(cozmo.objects.CustomObjectTypes.Custom_STAR5_Box, 10, 100, 200)

    time.sleep(100)

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
