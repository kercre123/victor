#!/usr/bin/env python3

'''
Test navigating to a pose
'''

import os
import sys
import time

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector  # pylint: disable=wrong-import-position


def main():
    '''main execution'''
    args = vector.util.parse_test_args()

    print("------ begin testing go to pose ------")

    # The robot should go to given pose
    with vector.Robot(args.name, args.ip, str(args.cert), port=args.port) as robot:

        pose = vector.util.Pose(x=50, y=0, z=0, angle_z=vector.util.Angle(degrees=0))
        robot.behavior.go_to_pose(pose)
        # Permit enough time to pass to reach required pose
        time.sleep(5)

        pose = vector.util.Pose(x=0, y=50, z=0, angle_z=vector.util.Angle(degrees=90))
        # Use a custom profile to change specs
        motion_profile = {"speed_mmps": 500.0, "is_custom": True}
        robot.behavior.go_to_pose(pose, motion_prof_map=motion_profile)
        # Permit enough to pass before the SDK mode is de-activated
        time.sleep(5)

    print("------ finished testing go to pose ------")


if __name__ == '__main__':
    main()
