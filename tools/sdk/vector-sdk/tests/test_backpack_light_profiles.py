#!/usr/bin/env python3

import asyncio

import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector

async def main(robot):

    # Set backpack to White Lights using the max brightness profile for 4 seconds
    await robot.set_all_backpack_lights( vector.lights.white_light, vector.lights.max_brightness_backpack_profile )
    await asyncio.sleep(2.5)

    # Set backpack to White Lights using the white balanced profile for 4 seconds
    await robot.set_all_backpack_lights( vector.lights.white_light, vector.lights.white_balanced_backpack_profile )
    await asyncio.sleep(2.5)

    # Set backpack to Magenta Lights using the max brightness profile for 4 seconds
    await robot.set_all_backpack_lights( vector.lights.magenta_light, vector.lights.max_brightness_backpack_profile )
    await asyncio.sleep(2.5)

    # Set backpack to Magenta Lights using the white balanced profile for 4 seconds
    await robot.set_all_backpack_lights( vector.lights.magenta_light, vector.lights.white_balanced_backpack_profile )
    await asyncio.sleep(2.5)

    await robot.set_all_backpack_lights( vector.lights.off_light )

if __name__ == "__main__":
    vector.robot.run_program(main, sys.argv[1] if len(sys.argv) > 1 else None)
