#!/usr/bin/env python3

import anki_vector  # pylint: disable=wrong-import-position


def main():
    args = anki_vector.util.parse_command_args()

    print("------ begin testing fetching robot stats ------")

    # Fetch robot stats
    with anki_vector.Robot(args.serial) as robot:
        battery_state = robot.get_battery_state()  # Fetch the battery level
        if battery_state:
            print("Robot battery_state: {0}".format(battery_state))

        robot.get_version_state()  # Fetch the os version and engine build version

    print("------ finished testing fetching robot stats ------")


if __name__ == "__main__":
    main()
