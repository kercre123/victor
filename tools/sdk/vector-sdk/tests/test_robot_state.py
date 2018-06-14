import asyncio
from pathlib import Path
import argparse
import time

import vector

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("ip")
    parser.add_argument("cert_file")
    args = parser.parse_args()

    cert = Path(args.cert_file)
    cert.resolve()

    print("------ begin testing actions ------")

    def test_subscriber(event_type, event):
        print(f"Subscriber called for: {event_type} = {event}")

    # Use the same loop to avoid closing it too early
    loop = asyncio.get_event_loop()

    print("------ Fetch robot state from robot's properties ------")
    with vector.Robot(args.ip, str(cert), loop=loop) as robot:
        '''
        Add some operation before testing properties to permit enough time
        for the stream to be setup
        '''
        robot.play_anim("anim_poked_giggle")
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