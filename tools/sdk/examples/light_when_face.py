#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

import asyncio
import time
import cozmo

'''This is a script to show off faces, and how they are easy to use.
It waits for a face, and then will light up his backpack when that face is visible.
'''

def run(coz_conn):
    coz = coz_conn.wait_for_robot()
    
    face = coz.world.wait_for_observed_face(timeout=30)
    
    while True:
        if face.is_face_visible():
            coz.set_all_backpack_lights(cozmo.lights.blue_light)
        else:
            coz.set_backpack_lights_off()
        time.sleep(.1)

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
