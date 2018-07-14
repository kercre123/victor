#!/usr/bin/env python3

import os
import sys
import time

import utilities
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector  # pylint: disable=wrong-import-position


def main():
    '''main execution'''
    args = utilities.parse_args()

    print("------ begin testing backpack light profiles ------")

    with vector.Robot(args.ip, str(args.cert), port=args.port) as robot:

        # Set backpack to White Lights using the max brightness profile for 4 seconds
        robot.backpack.set_all_backpack_lights(vector.lights.white_light, vector.lights.MAX_COLOR_PROFILE)
        time.sleep(4)

        # Set backpack to White Lights using the white balanced profile for 4 seconds
        robot.backpack.set_all_backpack_lights(vector.lights.white_light, vector.lights.WHITE_BALANCED_BACKPACK_PROFILE)
        time.sleep(4)

        # Set backpack to Magenta Lights using the max brightness profile for 4 seconds
        robot.backpack.set_all_backpack_lights(vector.lights.magenta_light, vector.lights.MAX_COLOR_PROFILE)
        time.sleep(4)

        # Set backpack to Magenta Lights using the white balanced profile for 4 seconds
        robot.backpack.set_all_backpack_lights(vector.lights.magenta_light, vector.lights.WHITE_BALANCED_BACKPACK_PROFILE)
        time.sleep(4)

        robot.backpack.set_all_backpack_lights(vector.lights.off_light)

    print("------ end testing backpack light profiles ------")


if __name__ == "__main__":
    main()
