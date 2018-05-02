#!/usr/bin/env python3

import math

import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector

async def main(robot):
    await robot.drive_straight(100.0)
    await robot.turn_in_place(math.pi*2)
    await robot.drive_straight(-100.0)

if __name__ == "__main__":
    vector.robot.run_program(main, sys.argv[1] if len(sys.argv) > 1 else None)
