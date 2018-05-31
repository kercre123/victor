#!/usr/bin/env python3

# This test should cycle victor's OLED 4 times between the solid colors:
# yellow-orange, green, azure, and a muted purple
#
# NOTE: Currently, victor's default eye animations will override the solid colors, so
#       when testing the colors will flicker back and forth between the eyes and color.
#
# @TODO: Once behaviors are worked out, we should be sending some kind of "stop default eye stuff"
#        message at the start of this program, and a complimentary "turn back on default eye stuff"
#        message when it stops.

import asyncio

import os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector

async def main(robot):
    for i in range(4):
        await robot.set_oled_to_color(vector.color.Color(rgb=[255, 128, 0]), duration_sec=1.0)
        await asyncio.sleep(1.0)
        await robot.set_oled_to_color(vector.color.Color(rgb=[48, 192, 48]), duration_sec=1.0)
        await asyncio.sleep(1.0)
        await robot.set_oled_to_color(vector.color.Color(rgb=[0, 128, 255]), duration_sec=1.0)
        await asyncio.sleep(1.0)
        await robot.set_oled_to_color(vector.color.Color(rgb=[96, 0, 192]), duration_sec=1.0)
        await asyncio.sleep(1.0)

if __name__ == "__main__":
    vector.robot.run_program(main, sys.argv[1] if len(sys.argv) > 1 else None)
