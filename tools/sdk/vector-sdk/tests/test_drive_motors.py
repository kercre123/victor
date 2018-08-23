#!/usr/bin/env python3

'''
Test wheel motor control functions
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

    with anki_vector.Robot(args.name, args.ip, str(args.cert), port=args.port) as robot:
        # manually drive about 0.1 m forward (100.0 mm/s for 1 sec) with a
        # 100.0 mm/s2 acceleration
        robot.motors.set_wheel_motors(100.0, 100.0, 100.0, 100.0)
        time.sleep(1.0)
        # stop moving and spin roughly all the way around (2xPi rad/s for 1
        # sec)
        robot.motors.set_wheel_motors(0.0, 0.0)
        robot.motors.set_wheel_motors_turn(100.0, curvature_radius_mm=(math.pi * 2))
        time.sleep(1.0)
        # stop turning
        robot.motors.set_wheel_motors_turn(0.0)
        # spin roughly all the way around (2xPi rad/s for 1 sec) with an
        # acceleration of 100.0 mm/s2
        robot.motors.set_wheel_motors_turn(100.0, turn_accel=100.0, curvature_radius_mm=(math.pi * 2))
        time.sleep(1.0)
        # stop turning and drive 0.1 m backward
        robot.motors.set_wheel_motors_turn(0.0)
        robot.motors.set_wheel_motors(-100.0, -100.0)
        time.sleep(1.0)
        # stop moving
        robot.motors.set_wheel_motors(0, 0)


if __name__ == "__main__":
    main()
