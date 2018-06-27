import argparse
import asyncio
import os
from pathlib import Path
import sys
import time
import functools

sys.path.append(os.path.join(os.path.dirname(__file__), '..'))
import vector


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("ip")
    parser.add_argument("cert_file")
    args = parser.parse_args()

    cert = Path(args.cert_file)
    cert.resolve()

    print("------ begin testing face events ------")
    # Should see a stream of face events when the robot observes a face

    def test_subscriber(robot, event_type, event):
        # Print the stream data received
        print(f"Subscriber called for: {event_type} = {event}")

        # Print the visible face's attributes
        for face in robot.world.visible_faces:
            print("Face attributes:")
            print(f"Face id: {face.face_id}")
            print(f"Updated face id: {face.updated_face_id}")
            print(f"Name: {face.name}")
            print(f"Expression: {face.expression}")
            print(f"Timestamp: {face.timestamp}")
            print(f"Pose: {face.pose}")
            print(f"Image Rect: {face.img_rect}")
            print(f"Expression score: {face.expression_score}")
            print(f"Left eye: {face.left_eye}")
            print(f"Right eye: {face.right_eye}")
            print(f"Nose: {face.nose}")
            print(f"Mouth: {face.mouth}")

    
    with vector.Robot(args.ip, str(cert)) as robot:
        test_subscriber = functools.partial(test_subscriber, robot)
        robot.events.subscribe('robot_changed_observed_face_id', test_subscriber)
        robot.events.subscribe('robot_observed_face', test_subscriber)
        
        print("------ waiting for face events, press ctrl+c to exit ------")
        try:
            robot.loop.run_forever()
        except KeyboardInterrupt:
            print("------ finished testing face events ------")
            robot.disconnect()


if __name__ == '__main__':
	main()