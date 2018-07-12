#!/usr/bin/env python3

'''
test_drive
'''
import os
import sys
import time

import utilities
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector  # pylint: disable=wrong-import-position
from vector.util import degrees, distance_mm, speed_mmps  # pylint: disable=wrong-import-position


def main():
    '''main execution'''
    args = utilities.parse_args()

    print("------ begin testing driving along a straight path and turning in place ------")

    # The robot shall drive straight, stop and then turn around
    with vector.Robot(args.ip, str(args.cert), port=args.port) as robot:
        robot.behavior.drive_straight(distance_mm(200), speed_mmps(50))
        time.sleep(2.0)  # Let enough time pass to drive straight

        robot.behavior.turn_in_place(degrees(180))
        time.sleep(2.0)  # Let enough time pass before SDK mode is de-activated

    print("------ finished testing driving along a straight path and turning in place ------")


if __name__ == "__main__":
    main()
