#!/usr/bin/env python3

'''
Test driving on and off charger
'''

import argparse
import math
import os
from pathlib import Path
import sys
import time

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("ip")
    parser.add_argument("cert_file")
    parser.add_argument("port")
    args = parser.parse_args()

    cert = Path(args.cert_file)
    cert.resolve()

    port = args.port

    with vector.Robot(args.ip, str(cert), str(port)) as robot:
        print("------ use low level motor controls to drive the robot ------")
        robot.set_wheel_motors(100.0, 100.0, 100.0, 100.0)
        time.sleep(5.0)
        robot.set_wheel_motors(0.0, 0.0)
        robot.set_wheel_motors_turn(100.0, curvature_radius_mm=(math.pi * 1))
        time.sleep(0.5)
        robot.set_wheel_motors_turn(0.0)
        time.sleep(5.0)

        print("------ begin testing drive on charger ------")
        result = robot.drive_on_charger()
        print("------ finish testing drive on charger. result: " + str(result))

        print("------ begin testing drive off charger ------")
        result = robot.drive_off_charger()
        print("------ finish testing drive off charger. result: " + str(result))

if __name__ == '__main__':
    main()
