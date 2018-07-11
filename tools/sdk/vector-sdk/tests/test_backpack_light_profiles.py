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

    print("------ begin testing backpack light profiles ------")

    with vector.Robot(args.ip, str(cert)) as robot:

        # Set backpack to White Lights using the max brightness profile for 4 seconds
        robot.set_all_backpack_lights( vector.lights.white_light, vector.lights.max_color_profile )
        time.sleep(4)

        # Set backpack to White Lights using the white balanced profile for 4 seconds
        robot.set_all_backpack_lights( vector.lights.white_light, vector.lights.white_balanced_backpack_profile )
        time.sleep(4)

        # Set backpack to Magenta Lights using the max brightness profile for 4 seconds
        robot.set_all_backpack_lights( vector.lights.magenta_light, vector.lights.max_color_profile )
        time.sleep(4)

        # Set backpack to Magenta Lights using the white balanced profile for 4 seconds
        robot.set_all_backpack_lights( vector.lights.magenta_light, vector.lights.white_balanced_backpack_profile )
        time.sleep(4)

        robot.set_all_backpack_lights( vector.lights.off_light )

    print("------ end testing backpack light profiles ------")

if __name__ == "__main__":
    main()
