#!/usr/bin/env python3

"""
Test proximity sensor data
"""

import time

import anki_vector  # pylint: disable=wrong-import-position


def main():
    args = anki_vector.util.parse_command_args()

    print("------ begin testing prox sensor data ------")

    with anki_vector.Robot(args.serial) as robot:
        for _ in range(30):
            proximity_data = robot.proximity.last_valid_sensor_reading
            if proximity_data is not None:
                print(proximity_data.distance)
            time.sleep(0.5)

    print("------ finish testing prox sensor data ------")


if __name__ == "__main__":
    main()
