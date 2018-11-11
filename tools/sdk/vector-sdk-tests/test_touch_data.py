#!/usr/bin/env python3

"""
Test touch sensor data
"""

import time

import anki_vector  # pylint: disable=wrong-import-position


def main():
    args = anki_vector.util.parse_command_args()

    print("------ begin testing touch sensor data ------")

    with anki_vector.Robot(args.serial) as robot:
        for _ in range(30):
            touch_data = robot.touch.last_sensor_reading
            if touch_data is not None:
                print('Touch sensor value: {0}, is being touched: {1}'.format(touch_data.raw_touch_value, touch_data.is_being_touched))
            else:
                print('No currently available touch data.')
            time.sleep(0.5)

    print("------ finish testing touch sensor data ------")


if __name__ == "__main__":
    main()
