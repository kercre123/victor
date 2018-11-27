#!/usr/bin/env python3

""" Generates wall/fixed object and pathfinds around it.
"""

import time

import anki_vector  # pylint: disable=wrong-import-position
from anki_vector.util import Pose, degrees
from anki_vector.objects import CustomObjectMarkers, CustomObjectTypes


def main():
    args = anki_vector.util.parse_command_args()

    with anki_vector.Robot(args.serial) as robot: # TODO We aren't enabling custom_object_detection yet
        fixed_object = robot.world.create_custom_fixed_object(Pose(100, 0, 0, angle_z=degrees(0)),
                                                              10, 100, 100, relative_to_robot=True)
        if fixed_object:
            print("fixed custom object created successfully")

        robot.behavior.go_to_pose(Pose(200, 0, 0, angle_z=degrees(0)), relative_to_robot=True)
        robot.world.delete_custom_objects()


if __name__ == "__main__":
    main()
