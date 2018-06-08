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

    with vector.Robot(args.ip, str(cert)) as robot:
        print("------ begin testing oled ------")

        for i in range(4):
            robot.set_oled_to_color(vector.color.Color(rgb=[255, 128, 0]), duration_sec=1.0)
            time.sleep(1.0)
            robot.set_oled_to_color(vector.color.Color(rgb=[48, 192, 48]), duration_sec=1.0)
            time.sleep(1.0)
            robot.set_oled_to_color(vector.color.Color(rgb=[0, 128, 255]), duration_sec=1.0)
            time.sleep(1.0)
            robot.set_oled_to_color(vector.color.Color(rgb=[96, 0, 192]), duration_sec=1.0)
            time.sleep(1.0)

        print("------ finish testing oled ------")

if __name__ == "__main__":
    main()
