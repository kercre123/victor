#!/usr/bin/env python3

import asyncio

import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector

async def main(robot):
    # move head upward for a second at an arbitrarily selected speed
    await robot.set_head_motor(5.0)
    await asyncio.sleep(1.0)
    # move head downward for a second at an arbitrarily selected speed
    await robot.set_head_motor(-5.0)
    await asyncio.sleep(1.0)
    # stop head movement
    await robot.set_head_motor(0.0)

    # move lift upward for a second at an arbitrarily selected speed
    await robot.set_lift_motor(5.0)
    await asyncio.sleep(1.0)
    # move lift downward for a second at an arbitrarily selected speed
    await robot.set_lift_motor(-5.0)
    await asyncio.sleep(1.0)
    # stop lift movement
    await robot.set_lift_motor(0.0)

if __name__ == "__main__":
    vector.robot.run_program(main, sys.argv[1] if len(sys.argv) > 1 else None)
