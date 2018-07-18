#!/usr/bin/env python3

import os
import sys

import utilities
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector  # pylint: disable=wrong-import-position


def main():
    '''main execution'''
    args = utilities.parse_args()

    print("------ begin testing fetching robot stats ------")

    # Fetch robot stats
    with vector.Robot(args.ip, str(args.cert), port=args.port) as robot:
        robot.get_battery_state()  # Fetch the battery level
        robot.get_version_state()  # Fetch the os version and engine build version
        robot.get_network_state()  # Fetch the network stats

    print("------ finished testing fetching robot stats ------")


if __name__ == "__main__":
    main()
