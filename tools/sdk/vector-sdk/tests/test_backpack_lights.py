#!/usr/bin/env python3

import asyncio

import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector

async def main(robot):

    # Set backpack to RGB Lights for 4 seconds
    await robot.set_backpack_lights( vector.lights.blue_light, vector.lights.green_light, vector.lights.red_light )
    await asyncio.sleep(4.0)

    # Set backpack to blinking yellow lights for 4 seconds (Warning lights)
    await robot.set_all_backpack_lights(
        vector.lights.Light(
            on_color=vector.lights.yellow,
            off_color=vector.lights.off,
            on_period_ms=100,
            off_period_ms=100 ) )
    await asyncio.sleep(4.0)

    # Set backpack to different shades of red using int codes for 4 seconds
    await robot.set_backpack_lights(
        vector.lights.Light( vector.lights.Color(int_color=0xff0000ff) ),
        vector.lights.Light( vector.lights.Color(int_color=0x1f0000ff) ),
        vector.lights.Light( vector.lights.Color(int_color=0x4f0000ff) ) )
    await asyncio.sleep(4.0)

    # Set backpack to some more complex colors using rgb for 4 seconds
    await robot.set_backpack_lights(
        vector.lights.Light( vector.lights.Color(rgb=(0,   128, 255)) ), # generic enterprise blue
        vector.lights.Light( vector.lights.Color(rgb=(192, 96,   48)) ), # a beige
        vector.lights.Light( vector.lights.Color(rgb=(96,  0,   192)) ) ) # a purple
    await asyncio.sleep(4.0)

    # Set backpack lights to fading between red and blue for 4 seconds (Police lights)
    await robot.set_all_backpack_lights(
        vector.lights.Light(
            on_color=vector.lights.red,
            off_color=vector.lights.blue,
            on_period_ms=25,
            off_period_ms=25,
            transition_on_period_ms=250,
            transition_off_period_ms=250 ) )
    await asyncio.sleep(4.0)

    # Turn off backpack lights
    await robot.set_all_backpack_lights(vector.lights.off_light)

if __name__ == "__main__":
    vector.robot.run_program(main, sys.argv[1] if len(sys.argv) > 1 else None)
