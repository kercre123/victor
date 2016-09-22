#!/usr/bin/env python3
# Copyright (c) 2016 Anki, Inc. All rights reserved. See LICENSE.txt for details.

import asyncio
import cozmo
from cozmo.util import degrees, Pose


def run(coz_conn):
    coz = coz_conn.wait_for_robot()
    
    coz.go_to_pose(Pose(100,100,0, angle_z=degrees(45)), relative_to_robot=True)

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)
