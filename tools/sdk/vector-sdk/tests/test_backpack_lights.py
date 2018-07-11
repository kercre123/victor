#!/usr/bin/env python3

import argparse
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
    args = parser.parse_args()

    cert = Path(args.cert_file)
    cert.resolve()

    print("------ begin testing backpack lights ------")

    with vector.Robot(args.ip, str(cert)) as robot:

        # Set backpack to RGB Lights for 4 seconds
        robot.set_backpack_lights( vector.lights.blue_light, vector.lights.green_light, vector.lights.red_light )
        time.sleep(4.0)

        # Set backpack to blinking yellow lights for 4 seconds (Warning lights)
        robot.set_all_backpack_lights(
            vector.lights.Light(
                on_color=vector.color.yellow,
                off_color=vector.color.off,
                on_period_ms=100,
                off_period_ms=100 ) )
        time.sleep(4.0)

        # Set backpack to different shades of red using int codes for 4 seconds
        robot.set_backpack_lights(
            vector.lights.Light( vector.color.Color(int_color=0xff0000ff) ),
            vector.lights.Light( vector.color.Color(int_color=0x1f0000ff) ),
            vector.lights.Light( vector.color.Color(int_color=0x4f0000ff) ) )
        time.sleep(4.0)

        # Set backpack to some more complex colors using rgb for 4 seconds
        robot.set_backpack_lights(
            vector.lights.Light( vector.color.Color(rgb=(0,   128, 255)) ), # generic enterprise blue
            vector.lights.Light( vector.color.Color(rgb=(192, 96,   48)) ), # a beige
            vector.lights.Light( vector.color.Color(rgb=(96,  0,   192)) ) ) # a purple
        time.sleep(4.0)

        # Set backpack lights to fading between red and blue for 4 seconds (Police lights)
        robot.set_all_backpack_lights(
            vector.lights.Light(
                on_color=vector.color.red,
                off_color=vector.color.blue,
                on_period_ms=25,
                off_period_ms=25,
                transition_on_period_ms=250,
                transition_off_period_ms=250 ) )
        time.sleep(4.0)

        # Turn off backpack lights
        robot.set_all_backpack_lights(vector.lights.off_light)

    print("------ end testing backpack lights ------")

if __name__ == "__main__":
    main()
