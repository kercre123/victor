#!/usr/bin/env python3

'''
Test driving on and off charger
'''

import math
import os
import sys
import time

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import anki_vector  # pylint: disable=wrong-import-position


def main():
    '''main execution'''
    args = anki_vector.util.parse_test_args()

    with anki_vector.Robot(args.name, args.ip, str(args.cert), args.port) as robot:
        print("------ use low level motor controls to drive the robot ------")
        robot.motors.set_wheel_motors(100.0, 100.0, 100.0, 100.0)
        time.sleep(5.0)
        robot.motors.set_wheel_motors(0.0, 0.0)
        robot.motors.set_wheel_motors_turn(100.0, curvature_radius_mm=(math.pi * 1))
        time.sleep(0.5)
        robot.motors.set_wheel_motors_turn(0.0)
        time.sleep(5.0)

        print("------ begin testing drive on charger ------")
        result = robot.behavior.drive_on_charger()
        print("------ finish testing drive on charger. result: " + str(result))

        print("------ begin testing drive off charger ------")
        result = robot.behavior.drive_off_charger()
        print("------ finish testing drive off charger. result: " + str(result))


if __name__ == '__main__':
    main()
