#!/usr/bin/env python3

from pathlib import Path
import time
import os
import sys
import argparse
import math
import asyncio

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector

def main():

    parser = argparse.ArgumentParser()
    parser.add_argument("ip")
    parser.add_argument("cert_file")
    args = parser.parse_args()

    cert = Path(args.cert_file)
    cert.resolve()
    
    loop = asyncio.get_event_loop()

    print("Testing synchronous robot's motor operations")
    with vector.Robot(args.ip, str(cert), loop=loop) as robot:
        # manually drive about 0.1 m forward (100.0 mm/s for 1 sec) with a 100.0 mm/s2 acceleration
        robot.set_wheel_motors(100.0, 100.0, 100.0, 100.0)
        time.sleep(1.0)
        # stop moving and spin roughly all the way around (2xPi rad/s for 1 sec)
        robot.set_wheel_motors(0.0, 0.0)
        robot.set_wheel_motors_turn(100.0, curvature_radius_mm=(math.pi * 2))
        time.sleep(1.0)
        # stop turning
        robot.set_wheel_motors_turn(0.0)
        # spin roughly all the way around (2xPi rad/s for 1 sec) with an acceleration of 100.0 mm/s2
        robot.set_wheel_motors_turn(100.0, turn_accel=100.0, curvature_radius_mm=(math.pi * 2))
        time.sleep(1.0)
        # stop turning and drive 0.1 m backward
        robot.set_wheel_motors_turn(0.0)
        robot.set_wheel_motors(-100.0, -100.0)
        time.sleep(1.0)
        # stop moving
        robot.set_wheel_motors(0, 0)


    print("Testing asynchronous robot's motor operations")
    with vector.AsyncRobot(args.ip, str(cert), loop=loop) as robot:
        # manually drive about 0.1 m forward (100.0 mm/s for 1 sec) with a 100.0 mm/s2 acceleration
        robot.set_wheel_motors(100.0, 100.0, 100.0, 100.0).wait_for_completed()
        time.sleep(1.0)
        # stop moving and spin roughly all the way around (2xPi rad/s for 1 sec)
        robot.set_wheel_motors(0.0, 0.0).wait_for_completed()
        robot.set_wheel_motors_turn(100.0, curvature_radius_mm=(math.pi * 2)).wait_for_completed()
        time.sleep(1.0)
        # stop turning
        robot.set_wheel_motors_turn(0.0).wait_for_completed()
        # spin roughly all the way around (2xPi rad/s for 1 sec) with an acceleration of 100.0 mm/s2
        robot.set_wheel_motors_turn(100.0, turn_accel=100.0, curvature_radius_mm=(math.pi * 2)).wait_for_completed()
        time.sleep(1.0)
        # stop turning and drive 0.1 m backward
        robot.set_wheel_motors_turn(0.0).wait_for_completed()
        robot.set_wheel_motors(-100.0, -100.0).wait_for_completed()
        time.sleep(1.0)
        # stop moving
        robot.set_wheel_motors(0.0, 0.0).wait_for_completed()

    loop.close()

if __name__ == "__main__":
    main()
