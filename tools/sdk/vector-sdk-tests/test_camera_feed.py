#!/usr/bin/env python3

"""
Test camera feed
"""

import time

import anki_vector  # pylint: disable=wrong-import-position


def main():
    args = anki_vector.util.parse_command_args()

    print("------ begin testing camera feed ------")

    # Receive camera feed from robot
    with anki_vector.Robot(args.serial, enable_camera_feed=True, show_viewer=True) as robot:
        print("------ waiting for image events, press ctrl+c to exit early ------")
        try:
            print("Render video for 10 seconds")
            time.sleep(10)

            print("Disabling video render for 5 seconds")
            robot.viewer.stop_video()
            time.sleep(5)

            print("Render video for 10 seconds")
            robot.viewer.show_video()
            time.sleep(10)

            print("Disabling video render and camera feed for 5 seconds")
            robot.viewer.stop_video()
            robot.enable_camera_feed = False
            time.sleep(5)

            print("Enabling video, after re-enabling camera feed for 10 seconds")
            robot.enable_camera_feed = True
            robot.viewer.show_video(timeout=5)
            time.sleep(10)

        except KeyboardInterrupt:
            print("------ image test aborted ------")

    print("------ finished testing camera feed ------")


if __name__ == '__main__':
    main()
