#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.
'''Make Cozmo sing Do Re Mi.


Slight extension from hello_world.py - introduces for loops to make Cozmo "sing" the scales.
'''

import sys

import cozmo
from cozmo.util import degrees

def run(coz_conn):
    '''The run method runs once Cozmo is connected.'''
    coz = coz_conn.wait_for_robot()

    scales = ["Doe", "Ray", "Mi", "Fa", "So", "La", "Ti", "Doe"]

    # Find voice_pitch_delta value that will range the pitch from -1 to 1 over all of the scales
    voice_pitch = -1.0
    voice_pitch_delta = 2.0 / (len(scales) -1)

    # Move head and lift down to the bottom, and wait until that's achieved
    coz.move_head(-5) # start moving head down so it mostly happens in parallel with lift
    coz.set_lift_height(0.0).wait_for_completed()
    coz.set_head_angle(degrees(-25.0)).wait_for_completed()

    # Start slowly raising lift and head
    coz.move_lift(0.15)
    coz.move_head(0.15)

    # "Sing" each note of the scale at increasingly high pitch
    for note in scales:
        coz.say_text(note, voice_pitch=voice_pitch, duration_scalar=0.3).wait_for_completed()
        voice_pitch += voice_pitch_delta

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    try:
        cozmo.connect(run)
    except cozmo.ConnectionError as e:
        sys.exit("A connection error occurred: %s" % e)
