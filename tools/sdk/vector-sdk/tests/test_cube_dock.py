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

    with anki_vector.Robot(args.name, args.ip, str(args.cert), port=args.port) as robot:
        robot.world.connect_cube()

        connected_cubes = robot.world.connected_light_cubes
        if connected_cubes:
            robot.behavior.dock_with_cube(connected_cubes[0])

    print("------ finish cube docking ------")


if __name__ == "__main__":
    main()
