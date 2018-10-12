#!/usr/bin/env python3

"""
"""

import anki_vector  # pylint: disable=wrong-import-position
from anki_vector.util import Pose, degrees
import asyncio


def main():
    """main execution"""
    args = anki_vector.util.parse_command_args()

    with anki_vector.Robot(args.serial) as robot:
        robot.loop.run_until_complete(asyncio.wait_for(asyncio.sleep(1), 10.0))
        print(robot.pose)

        fixed_object = robot.world.create_custom_fixed_object(Pose(100, 0, 0, angle_z=degrees(0)),
                                                              10, 100, 100, relative_to_robot=True)
        if fixed_object:
            print("fixed_object created successfully")

        robot.behavior.go_to_pose(Pose(200, 0, 0, angle_z=degrees(0)), relative_to_robot=True)

        print('objects: {0}'.format(robot.world.all_objects))

if __name__ == "__main__":
    main()
