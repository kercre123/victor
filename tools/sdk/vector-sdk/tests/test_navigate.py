#!/usr/bin/env python3

import argparse
import asyncio
import os
from pathlib import Path
import sys
import time

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector
from vector import util

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("ip")
    parser.add_argument("cert_file")
    args = parser.parse_args()

    cert = Path(args.cert_file)
    cert.resolve()

    print("------ begin testing go to pose ------")

    # The robot should go to given pose
    with vector.Robot(args.ip, str(cert)) as robot:

        pose = util.Pose(x=50, y=0, z=0, angle_z=util.Angle(degrees=0))
        robot.go_to_pose(pose)
        time.sleep(5) # Permit enough time to pass to reach required pose

        pose = util.Pose(x=0, y=50, z=0, angle_z=util.Angle(degrees=90))
        motion_profile = {"speed_mmps": 500.0, "is_custom": True} # Use a custom profile to change specs
        robot.go_to_pose(pose, motion_prof_map=motion_profile)
        time.sleep(5) # Permit enough to pass before the SDK mode is de-activated

    print("------ finished testing go to pose ------")


if __name__ == '__main__':
    main()