#!/usr/bin/env python3

'''
Test the motors for the head and lift
'''

import os
import sys
import time

import utilities
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector  # pylint: disable=wrong-import-position


def main():
    '''main execution'''
    args = utilities.parse_args()

    print("------ begin testing head and lift motors ------")
    with vector.Robot(args.ip, str(args.cert), port=args.port) as robot:
        # move head upward for a second at an arbitrarily selected speed
        robot.motors.set_head_motor(5.0)
        time.sleep(1.0)
        # move head downward for a second at an arbitrarily selected speed
        robot.motors.set_head_motor(-5.0)
        time.sleep(1.0)
        # stop head movement
        robot.motors.set_head_motor(0)
        # move lift upward for a second at an arbitrarily selected speed
        robot.motors.set_lift_motor(5.0)
        time.sleep(1.0)
        # move lift downward for a second at an arbitrarily selected speed
        robot.motors.set_lift_motor(-5.0)
        time.sleep(1.0)
        # stop lift movement
        robot.motors.set_lift_motor(0.0)
    print("------ finished testing head and lift motors ------")


if __name__ == "__main__":
    main()
