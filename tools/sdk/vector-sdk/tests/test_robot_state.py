#!/usr/bin/env python3

'''
Test the robot state
'''

import os
import sys

import utilities
sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector  # pylint: disable=wrong-import-position


def main():
    '''main execution'''
    args = utilities.parse_args()

    print("------ Fetch robot state from robot's properties ------")
    with vector.Robot(args.name, args.ip, str(args.cert), port=args.port) as robot:
        # Add some operation before testing properties to permit enough time
        # for the stream to be setup
        robot.anim.play_animation("anim_poked_giggle")
        print(robot.pose)
        print(robot.pose_angle_rad)
        print(robot.pose_pitch_rad)
        print(robot.left_wheel_speed_mmps)
        print(robot.right_wheel_speed_mmps)
        print(robot.head_angle_rad)
        print(robot.lift_height_mm)
        print(robot.battery_voltage)
        print(robot.accel)
        print(robot.gyro)
        print(robot.carrying_object_id)
        print(robot.carrying_object_on_top_id)
        print(robot.head_tracking_object_id)
        print(robot.localized_to_object_id)
        print(robot.last_image_time_stamp)
        print(robot.status)
        print(robot.game_status)


if __name__ == '__main__':
    main()
