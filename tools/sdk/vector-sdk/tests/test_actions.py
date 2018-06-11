#!/usr/bin/env python3

'''
Test actions to make sure they work properly in all combinations:
* Synchronous robot using with syntax
* Synchronous robot using try finally syntax
* Asynchronous robot using with syntax
* Asynchronous robot using try finally syntax
* Lastly disconnecting and reconnecting with the same robot
'''

import asyncio
from pathlib import Path
import argparse
import time

import vector

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("ip")
    parser.add_argument("cert_file")
    args = parser.parse_args()

    cert = Path(args.cert_file)
    cert.resolve()

    print("------ begin testing actions ------")

    def test_subscriber(event_type, event):
        print(f"Subscriber called for: {event_type} = {event}")
    # Use the same loop to avoid closing it too early
    loop = asyncio.get_event_loop()

    print("------ Synchronous Robot using with ------")
    with vector.Robot(args.ip, str(cert), loop=loop) as robot:
        robot.events.subscribe('robot_state', test_subscriber)
        robot.play_anim("anim_poked_giggle")

    print("------ Synchronous Robot using with ------")
    with vector.Robot(args.ip, str(cert), loop=loop) as robot:
        robot.events.subscribe('test1', test_subscriber)
        robot.play_anim("anim_poked_giggle")
        robot.events.unsubscribe('test1', test_subscriber)
        robot.events.unsubscribe('test1', test_subscriber)
        robot.set_wheel_motors(100.0, 100.0)

    time.sleep(2)

    print("------ Synchronous Robot using try finally ------")
    robot = vector.Robot(args.ip, str(cert), loop=loop)
    robot.events.subscribe('test1', test_subscriber)
    try:
        robot.connect()
        robot.play_anim("anim_poked_giggle")
        robot.set_wheel_motors(100.0, 100.0)
    finally:
        robot.disconnect()

    time.sleep(2)

    print("------ Asynchronous Robot using with ------")
    with vector.AsyncRobot(args.ip, str(cert), loop=loop) as robot:
        robot.events.subscribe('robot_state', test_subscriber)
        robot.play_anim("anim_poked_giggle").wait_for_completed()
        robot.set_wheel_motors(100.0, 100.0).wait_for_completed()

    time.sleep(2)

    print("------ Asynchronous Robot using try finally ------")
    robot = vector.AsyncRobot(args.ip, str(cert), loop=loop)
    robot.events.subscribe('test1', test_subscriber)
    try:
        robot.connect()
        robot.play_anim("anim_poked_giggle").wait_for_completed()
        robot.set_wheel_motors(100.0, 100.0).wait_for_completed()
    finally:
        robot.disconnect()

    time.sleep(2)

    print("------ Repeated Robot using try finally ------")
    # Reuse the same robot from a previous connection
    try:
        robot.connect()
        robot.play_anim("anim_poked_giggle").wait_for_completed()
        robot.set_wheel_motors(100.0, 100.0).wait_for_completed()
    finally:
        robot.disconnect()

    loop.close()
    print("------ finish testing actions ------")

if __name__ == "__main__":
    main()
