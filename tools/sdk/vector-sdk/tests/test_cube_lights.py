#!/usr/bin/env python3

import asyncio

import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector

async def main(robot):

    # Set cube lights to yellow
    await robot.set_cube_lights( 1, vector.lights.yellow_light )
    await asyncio.sleep(2.5)

    # Set cube lights to red, green, blue, and white
    await robot.set_cube_light_corners( 1, vector.lights.blue_light, vector.lights.green_light, vector.lights.red_light, vector.lights.white_light )
    await asyncio.sleep(2.5)

    await robot.set_cube_lights( 1, vector.lights.off_light )

if __name__ == "__main__":
    vector.robot.run_program(main, sys.argv[1] if len(sys.argv) > 1 else None)
