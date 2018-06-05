#!/usr/bin/env python3

from pathlib import Path
import time
import os
import sys
import argparse
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

    print("Testing synchronous robot's head and lift motor operations")
    with vector.Robot(args.ip, str(cert), loop=loop) as robot:
        # move head upward for a second at an arbitrarily selected speed
        robot.set_head_motor(5.0)
        time.sleep(1.0)
        # move head downward for a second at an arbitrarily selected speed
        robot.set_head_motor(-5.0)
        time.sleep(1.0)
        # stop head movement
        robot.set_head_motor(0)
        # move lift upward for a second at an arbitrarily selected speed
        robot.set_lift_motor(5.0)
        time.sleep(1.0)
        # move lift downward for a second at an arbitrarily selected speed
        robot.set_lift_motor(-5.0)
        time.sleep(1.0)
        # stop lift movement
        robot.set_lift_motor(0.0)

    print("Testing asynchronous robot's head and lift motor operations")
    with vector.AsyncRobot(args.ip, str(cert), loop=loop) as robot:
        # move head upward for a second at an arbitrarily selected speed
        robot.set_head_motor(5.0).wait_for_completed()
        time.sleep(1.0)
        # move head downward for a second at an arbitrarily selected speed
        robot.set_head_motor(-5.0).wait_for_completed()
        time.sleep(1.0)
        # stop head movement
        robot.set_head_motor(0).wait_for_completed()
        # move lift upward for a second at an arbitrarily selected speed
        robot.set_lift_motor(5.0).wait_for_completed()
        time.sleep(1.0)
        # move lift downward for a second at an arbitrarily selected speed
        robot.set_lift_motor(-5.0).wait_for_completed()
        time.sleep(1.0)
        # stop lift movement
        robot.set_lift_motor(0.0).wait_for_completed()

    loop.close()

if __name__ == "__main__":
    main()
