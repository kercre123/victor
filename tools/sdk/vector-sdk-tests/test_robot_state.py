#!/usr/bin/env python3

"""
Test the robot state
"""

import time

import anki_vector  # pylint: disable=wrong-import-position


def main():
    args = anki_vector.util.parse_command_args()

    print("------ Fetch robot state from robot's properties ------")
    with anki_vector.Robot(args.serial) as robot:
        time.sleep(1)
        print(robot.pose)
        print(robot.pose_angle_rad)
        print(robot.pose_pitch_rad)
        print(robot.left_wheel_speed_mmps)
        print(robot.right_wheel_speed_mmps)
        print(robot.head_angle_rad)
        print(robot.lift_height_mm)
        print(robot.accel)
        print(robot.gyro)
        print(robot.carrying_object_id)
        print(robot.head_tracking_object_id)
        print(robot.localized_to_object_id)
        print(robot.last_image_time_stamp)
        print(robot.status)


if __name__ == '__main__':
    main()
