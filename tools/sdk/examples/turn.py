#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

import asyncio

import cozmo
from cozmo.util import degrees


def run(coz_conn):
    coz = coz_conn.wait_for_robot()

    # Turn 90 degrees, play an animation, exit.
    coz.turn_in_place(degrees(90)).wait_for_completed()

    anim = coz.play_anim_trigger(cozmo.anim.Triggers.MajorWin)
    anim.wait_for_completed()


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
