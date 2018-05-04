#!/usr/bin/env python3

import math
import asyncio

import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector

async def main(robot):
    # manually drive about 0.1 m forward (100.0 mm/s for 1 sec)
    await robot.set_wheel_motors(100.0, 100.0)
    await asyncio.sleep(1.0)
    # stop moving and spin roughly all the way around (2xPi rad/s for 1 sec)
    await robot.set_wheel_motors(0.0, 0.0)
    await robot.set_wheel_motors_turn(math.pi * 2)
    await asyncio.sleep(1.0)
    # stop turning and drive 0.1 m backward
    await robot.set_wheel_motors_turn(0.0)
    await robot.set_wheel_motors(-100.0, -100.0)
    await asyncio.sleep(1.0)
    await robot.set_wheel_motors(0.0, 0.0)

if __name__ == "__main__":
    vector.robot.run_program(main, sys.argv[1] if len(sys.argv) > 1 else None)
