#!/usr/bin/env python3

'''
Test the setting angle and height for the head and lift respectively
'''

import os
import sys
import time

import utilities
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector  # pylint: disable=wrong-import-position
from vector.util import degrees  # pylint: disable=wrong-import-position


def main():
    '''main execution'''
    args = utilities.parse_args()

    print("------ begin testing head and lift actions ------")

    # The robot shall lower and raise his head and lift
    with vector.Robot(args.ip, str(args.cert), port=args.port) as robot:
        robot.behavior.set_head_angle(degrees(-50.0))
        time.sleep(2)

        robot.behavior.set_head_angle(degrees(50.0))
        time.sleep(2)

        robot.behavior.set_head_angle(degrees(0.0))
        time.sleep(2)

        robot.behavior.set_lift_height(100.0)
        time.sleep(2)

        robot.behavior.set_lift_height(0.0)
        time.sleep(2)

    print("------ finished testing head and lift actions ------")


if __name__ == "__main__":
    main()
