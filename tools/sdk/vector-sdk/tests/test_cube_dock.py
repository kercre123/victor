#!/usr/bin/env python3

"""
Test cube docking behavior.

Vector should drive to a seen cube, lining up so that his lift hooks are inserted into
the cube's corners.
"""

import os
import sys

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import anki_vector  # pylint: disable=wrong-import-position


def main():
    """main execution"""
    args = anki_vector.util.parse_test_args()

    print("------ begin cube docking ------")

    docking_result_code = -1
    with anki_vector.Robot(args.serial, port=args.port) as robot:
        robot.world.connect_cube()

        connected_cubes = robot.world.connected_light_cubes
        if connected_cubes:
            dock_response = robot.behavior.dock_with_cube(connected_cubes[0])
            docking_result_code = dock_response.result

    if docking_result_code == anki_vector.messaging.protocol.ActionResult.ACTION_RESULT_SUCCESS:  # pylint: disable=no-member
        print("------ finish cube docking ------")
    else:
        print("------ cube docking failed with code {0} ------".format(docking_result_code))


if __name__ == "__main__":
    main()
